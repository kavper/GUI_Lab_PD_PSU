/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.22.1. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include <TouchGFXHAL.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */
#include "FreeRTOS.h"
#include <nema_graphics.h>
#include <nema_blender.h>
#include <cstring>
#include <stm32h7rsxx_hal.h>
#include "main.h"
#include <gui/common/DisplayComposition.hpp>

using namespace touchgfx;

namespace
{
// Capacity for the planned panel. Active pitch/rotation use Designer dimensions.
constexpr unsigned maxDisplayWidth = 1024, maxDisplayHeight = 600;
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint32_t animationBuffer[maxDisplayWidth * maxDisplayHeight / 2] LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
// Separate scanout buffers preserve TouchGFX's unrotated double buffering,
// including partial redraws, animation storage and framebuffer copies.
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint32_t rotatedBuffers[2][maxDisplayWidth * maxDisplayHeight / 2] LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint32_t bitmapCache[2 * 1024 * 1024 / 4] LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint32_t transitionTextures[2][maxDisplayWidth * maxDisplayHeight / 2] LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
uint16_t* volatile displayedLogicalBuffer = nullptr;
uint32_t frameStartCycles;
Rect currentDirty;
Rect previousDirty;
}

DisplayComposition boardDisplayComposition = {};
void BoardDisplayPrepare(const uint16_t* outgoing, const uint16_t* incoming)
{
    auto* hal=touchgfx::HALGPU2D::getInstance();
    const unsigned width=touchgfx::HAL::DISPLAY_WIDTH, height=touchgfx::HAL::DISPLAY_HEIGHT;
    // Finish the outgoing snapshot DMA and destination bitmap render first.
    hal->lockFrameBuffer(); hal->unlockFrameBuffer();
    HAL_ICACHE_Invalidate();
    for(unsigned n=0;n<2;n++) {
        nema_bind_dst_tex(reinterpret_cast<uintptr_t>(transitionTextures[n]),width,height,NEMA_RGB565,width*2);
        nema_bind_src_tex(reinterpret_cast<uintptr_t>(n?incoming:outgoing),width,height,NEMA_RGB565,width*2,NEMA_FILTER_PS);
        nema_set_clip(0,0,width,height); nema_set_blend_blit(NEMA_BL_SRC);
        nema_blit_rotate(0,0,NEMA_ROT_180_CW);
    }
    hal->submitExecBuffer();
    hal->lockFrameBuffer(); hal->unlockFrameBuffer();
}

extern "C" {
volatile uint32_t BoardDisplayRotationFrames = 0;
// SWD profiling: frame count, total/max/last render us, previous begin,
// last/max begin period us. Benchmark forces full redraws from the GUI task.
volatile uint32_t BoardGraphicsProfile[7] = {};
volatile uint32_t BoardGraphicsBenchmark = 0;
volatile uint32_t BoardGraphicsCpuUs = 0;
volatile uint32_t BoardGraphicsFinishUs = 0;
volatile uint32_t BoardGraphicsSkipRotation = 0;
volatile uint32_t BoardGraphicsSwipeBenchmark = 0;
volatile uint32_t BoardGraphicsMotion = 0;
volatile uint32_t BoardGraphicsFrameTimes[128][3] = {};
}

void TouchGFXHAL::initialize()
{
    // Calling parent implementation of initialize().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.
    // Please note, HAL::initialize() must be called to initialize the framework.

    TouchGFXGeneratedHAL::initialize();
    if (DISPLAY_WIDTH > maxDisplayWidth || DISPLAY_HEIGHT > maxDisplayHeight) Error_Handler();
    // 128 corner palettes x 4 textures plus transition bitmaps.
    Bitmap::setCache(reinterpret_cast<uint16_t*>(bitmapCache), sizeof(bitmapCache), 516);
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    displayedLogicalBuffer = frameBuffer0;
    std::memset(rotatedBuffers, 0, sizeof(rotatedBuffers));
    FlushCache();
    TouchGFXGeneratedHAL::setTFTFrameBuffer(reinterpret_cast<uint16_t*>(rotatedBuffers[0]));
    setAnimationStorage((void*)animationBuffer);  //enable the animation storage to allow slide animations
    instrumentation.init();
    setMCUInstrumentation(&instrumentation);
    enableMCULoadCalculation(true);
}

/**
 * Gets the frame buffer address used by the TFT controller.
 *
 * @return The address of the frame buffer currently being displayed on the TFT.
 */
uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    // Calling parent implementation of getTFTFrameBuffer().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    return displayedLogicalBuffer;
}

/**
 * Sets the frame buffer address used by the TFT controller.
 *
 * @param [in] address New frame buffer address.
 */
void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    // Calling parent implementation of setTFTFrameBuffer(uint16_t* address).
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    // Called at VSYNC: only change the scanout address here. GPU work is
    // completed in the GUI task before the framework requests a swap.
    displayedLogicalBuffer = address;
    const unsigned index = address == frameBuffer0 ? 0 : 1;
    TouchGFXGeneratedHAL::setTFTFrameBuffer(reinterpret_cast<uint16_t*>(rotatedBuffers[index]));
}

/**
 * This function is called whenever the framework has performed a partial draw.
 *
 * @param rect The area of the screen that has been drawn, expressed in absolute coordinates.
 *
 * @see flushFrameBuffer().
 */
void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    // Calling parent implementation of flushFrameBuffer(const touchgfx::Rect& rect).
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.
    // Please note, HAL::flushFrameBuffer(const touchgfx::Rect& rect) must
    // be called to notify the touchgfx framework that flush has been performed.
    // To calculate he start adress of rect,
    // use advanceFrameBufferToRect(uint8_t* fbPtr, const touchgfx::Rect& rect)
    // defined in TouchGFXGeneratedHAL.cpp

    currentDirty.expandToFit(rect);
    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
    if (boardDisplayComposition.active) {
        const uintptr_t d=reinterpret_cast<uintptr_t>(dest),s=reinterpret_cast<uintptr_t>(src);
        const uintptr_t f0=reinterpret_cast<uintptr_t>(frameBuffer0),f1=reinterpret_cast<uintptr_t>(frameBuffer1);
        const uint32_t size=DISPLAY_WIDTH*DISPLAY_HEIGHT*2;
        if (((d>=f0 && d+numBytes<=f0+size)||(d>=f1 && d+numBytes<=f1+size)) &&
            ((s>=f0 && s+numBytes<=f0+size)||(s>=f1 && s+numBytes<=f1+size))) return true;
    }
    return TouchGFXGeneratedHAL::blockCopy(dest, src, numBytes);
}

/**
 * Configures the interrupts relevant for TouchGFX. This primarily entails setting
 * the interrupt priorities for the DMA and LCD interrupts.
 */
void TouchGFXHAL::configureInterrupts()
{
    // Calling parent implementation of configureInterrupts().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    TouchGFXGeneratedHAL::configureInterrupts();
}

/**
 * Used for enabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::enableInterrupts()
{
    // Calling parent implementation of enableInterrupts().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    TouchGFXGeneratedHAL::enableInterrupts();
}

/**
 * Used for disabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::disableInterrupts()
{
    // Calling parent implementation of disableInterrupts().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    TouchGFXGeneratedHAL::disableInterrupts();
}

/**
 * Configure the LCD controller to fire interrupts at VSYNC. Called automatically
 * once TouchGFX initialization has completed.
 */
void TouchGFXHAL::enableLCDControllerInterrupt()
{
    // Calling parent implementation of enableLCDControllerInterrupt().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    TouchGFXGeneratedHAL::enableLCDControllerInterrupt();
}

bool TouchGFXHAL::beginFrame()
{
    const bool started = TouchGFXGeneratedHAL::beginFrame();
    if (started) {
        currentDirty = Rect();
        frameStartCycles = DWT->CYCCNT;
        uint32_t period = (frameStartCycles - BoardGraphicsProfile[4]) / 600;
        BoardGraphicsProfile[4] = frameStartCycles;
        BoardGraphicsProfile[5] = period;
        if (period > BoardGraphicsProfile[6]) BoardGraphicsProfile[6] = period;
    }
    return started;
}

void TouchGFXHAL::endFrame()
{
    const bool updated = frameBufferUpdatedThisFrame;
    const uint32_t finishStart = DWT->CYCCNT;
    BoardGraphicsCpuUs = (finishStart - frameStartCycles) / 600;
    if (frameBufferUpdatedThisFrame && !BoardGraphicsSkipRotation)
    {
        uint16_t* source = getClientFrameBuffer();
        const unsigned index = source == frameBuffer0 ? 0 : 1;
        if (!boardDisplayComposition.active) InvalidateTextureCache();
        // Append rotation after drawing commands in the same GPU list. The
        // parent endFrame waits for completion before allowing a VSYNC swap.
        const unsigned width = DISPLAY_WIDTH, height = DISPLAY_HEIGHT;
        nema_bind_dst_tex(reinterpret_cast<uintptr_t>(rotatedBuffers[index]), width, height, NEMA_RGB565, width * 2);
        if (boardDisplayComposition.active) {
            for (unsigned n=0;n<2;n++) {
                const int logicalX=n?boardDisplayComposition.incomingX:boardDisplayComposition.outgoingX;
                Rect logical(logicalX,0,width,height);
                if (n) logical &= boardDisplayComposition.incomingClip;
                else if (!logicalX && !boardDisplayComposition.incomingX &&
                         !boardDisplayComposition.incomingClip.isEmpty()) {
                    // A wipe partitions the screen; avoid copying the outgoing
                    // pixels that will immediately be covered by the new view.
                    const Rect& revealed=boardDisplayComposition.incomingClip;
                    if (!revealed.x) { logical.x=revealed.right(); logical.width=width-logical.x; }
                    else logical.width=revealed.x;
                }
                logical &= Rect(0,0,width,height);
                if (logical.isEmpty()) continue;
                const int dx=width-logical.right(), dy=height-logical.bottom();
                const int textureX=width-(logical.x-logicalX)-logical.width;
                const int textureY=height-logical.y-logical.height;
                const auto* pixels=reinterpret_cast<const uint16_t*>(transitionTextures[n])+textureY*width+textureX;
                nema_bind_src_tex(reinterpret_cast<uintptr_t>(pixels),logical.width,logical.height,NEMA_RGB565,width*2,NEMA_FILTER_PS);
                nema_set_clip(0,0,width,height); nema_set_blend_blit(NEMA_BL_SRC);
                nema_blit(dx,dy);
            }
            previousDirty=Rect(0,0,width,height);
        } else {
        // Each logical buffer also receives the previous frame's dirty areas
        // during TouchGFX double-buffer synchronization. Rotate their union.
        Rect dirty = currentDirty;
        dirty.expandToFit(previousDirty);
        if (dirty.isEmpty()) dirty = Rect(0, 0, width, height);
        nema_bind_src_tex(reinterpret_cast<uintptr_t>(source + dirty.y * width + dirty.x),
                          dirty.width, dirty.height, NEMA_RGB565, width * 2, NEMA_FILTER_PS);
        nema_set_clip(0, 0, width, height);
        nema_set_blend_blit(NEMA_BL_SRC);
        nema_blit_rotate(width - dirty.x - dirty.width, height - dirty.y - dirty.height, NEMA_ROT_180_CW);
        previousDirty = currentDirty;
        }
        submitExecBuffer();
        ++BoardDisplayRotationFrames;
    }
    TouchGFXGeneratedHAL::endFrame();
    BoardGraphicsFinishUs = (DWT->CYCCNT - finishStart) / 600;
    if (updated) {
        // Direct composition deliberately leaves logical buffers unchanged.
        // Rebuild both after returning to live widgets, even if the final
        // eased position was already reached on the penultimate tick.
        if (!boardDisplayComposition.active && boardDisplayComposition.resumeFrames)
            --boardDisplayComposition.resumeFrames;
        if (BoardGraphicsBenchmark && --BoardGraphicsBenchmark == 0) BoardGraphicsSkipRotation = 0;
        const uint32_t elapsed = (DWT->CYCCNT - frameStartCycles) / 600;
        const unsigned sample = BoardGraphicsProfile[0] % 128;
        BoardGraphicsFrameTimes[sample][0] = elapsed;
        BoardGraphicsFrameTimes[sample][1] = BoardGraphicsProfile[5];
        BoardGraphicsFrameTimes[sample][2] = BoardGraphicsMotion;
        ++BoardGraphicsProfile[0]; BoardGraphicsProfile[1] += elapsed;
        if (elapsed > BoardGraphicsProfile[2]) BoardGraphicsProfile[2] = elapsed;
        BoardGraphicsProfile[3] = elapsed;
    }
}

extern "C"
{
    portBASE_TYPE IdleTaskHook(void* p)
    {
        if ((int)p) //idle task sched out
        {
            touchgfx::HAL::getInstance()->setMCUActive(true);
        }
        else //idle task sched in
        {
            touchgfx::HAL::getInstance()->setMCUActive(false);
        }
        return pdTRUE;
    }
}
/* USER CODE END TouchGFXHAL.cpp */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
