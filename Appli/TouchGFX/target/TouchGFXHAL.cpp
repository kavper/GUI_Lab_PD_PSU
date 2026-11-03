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
uint16_t* volatile displayedLogicalBuffer = nullptr;
uint32_t frameStartCycles;
Rect currentDirty;
Rect previousDirty;
const uint16_t* transitionLogicalSource = nullptr;
const uint16_t* transitionOutgoingSource = nullptr;
unsigned transitionLogicalCopies = 0;
bool logicalBuffersSynchronized = false;
}

DisplayComposition boardDisplayComposition = {};
extern "C" { volatile uint32_t BoardGraphicsTransitionUs[4] = {}; }
uint32_t BoardDisplayCycles() { return DWT->CYCCNT; }
void BoardDisplayMeasure(unsigned stage,uint32_t started)
{
    if(stage<4) BoardGraphicsTransitionUs[stage]=(DWT->CYCCNT-started)/600;
}
void BoardDisplayPrepare(const uint16_t* outgoing, const uint16_t* incoming)
{
    const uint32_t started=BoardDisplayCycles();
    transitionLogicalSource=incoming;
    transitionOutgoingSource=outgoing;
    transitionLogicalCopies=0;
    auto* hal=touchgfx::HALGPU2D::getInstance();
    // Finish the outgoing snapshot DMA and destination bitmap render first.
    hal->lockFrameBuffer(); hal->unlockFrameBuffer();
    HAL_ICACHE_Invalidate();
    BoardDisplayMeasure(1,started);
    // Rotate each visible source region directly into scanout. Pre-rotating
    // both full snapshots here added two unnecessary PSRAM read/write passes.
}

bool BoardDisplayFinish()
{
    if (!boardDisplayComposition.active) return false;
    const bool restored=transitionLogicalCopies==8;
    boardDisplayComposition.active=false;
    boardDisplayComposition.preparing=false;
    boardDisplayComposition.resumeFrames=restored?0:2;
    logicalBuffersSynchronized=restored;
    if(restored)previousDirty=Rect(); // Both scanout buffers contain the endpoint.
    transitionLogicalSource=nullptr;
    return restored;
}
bool BoardDisplayReady() { return transitionLogicalCopies==8; }

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
volatile uint32_t BoardGraphicsRouteBenchmark = 0;
volatile uint32_t BoardGraphicsMotion = 0;
volatile uint32_t BoardGraphicsFrameTimes[128][3] = {};
volatile uint32_t BoardGraphicsPreparationProfile[4] = {};
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
    // 128 corner palettes x 4 textures, 64 tinted icons and transition bitmaps.
    Bitmap::setCache(reinterpret_cast<uint16_t*>(bitmapCache), sizeof(bitmapCache), 580);
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
    if (boardDisplayComposition.active || logicalBuffersSynchronized) {
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
    if(boardDisplayComposition.preparing) {
        // The displayed scanout stays intact while bounded offscreen work is
        // completed. Continue servicing touch/model ticks; do not swap an
        // unfinished snapshot or rotate unchanged full screens in this phase.
        frameBufferUpdatedThisFrame=false;
        TouchGFXGeneratedHAL::endFrame();
        const uint32_t elapsed=(DWT->CYCCNT-frameStartCycles)/600;
        ++BoardGraphicsPreparationProfile[0];BoardGraphicsPreparationProfile[1]+=elapsed;
        if(elapsed>BoardGraphicsPreparationProfile[2])BoardGraphicsPreparationProfile[2]=elapsed;
        BoardGraphicsPreparationProfile[3]=elapsed;
        return;
    }
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
        const DisplayComposition& c=boardDisplayComposition;
        // The outgoing logical buffer may only be overwritten once nothing of
        // the outgoing layer is visible any more.
        const bool outgoingGone=!c.outgoingOnTop ||
            c.outgoingX<=-(int)width || c.outgoingX>=(int)width ||
            c.outgoingY<=-(int)height || c.outgoingY>=(int)height;
        const bool destinationCoversScreen=c.incomingX==0 && c.incomingY==0 && outgoingGone &&
            c.incomingClip==Rect(0,0,width,height);
        if (boardDisplayComposition.active && transitionLogicalCopies<8 &&
            (transitionLogicalCopies<4 || destinationCoversScreen)) {
            // Reuse the destination already rendered for the animation. Spread
            // the two logical-buffer copies over eight frames, while neither is
            // scanned out. This avoids two complete widget redraws on release.
            const unsigned strip=transitionLogicalCopies%4;
            const unsigned y=strip*height/4, rows=(strip+1)*height/4-y;
            // The displayed logical buffer itself is the outgoing snapshot.
            // Preserve it until the destination covers it completely; restore
            // the other logical buffer first, then this one in endpoint frames.
            auto* outgoingBuffer=const_cast<uint16_t*>(transitionOutgoingSource);
            auto* otherBuffer=outgoingBuffer==frameBuffer0?frameBuffer1:frameBuffer0;
            nema_bind_dst_tex(reinterpret_cast<uintptr_t>(transitionLogicalCopies>=4?outgoingBuffer:otherBuffer),width,height,NEMA_RGB565,width*2);
            nema_bind_src_tex(reinterpret_cast<uintptr_t>(transitionLogicalSource+y*width),width,rows,NEMA_RGB565,width*2,NEMA_FILTER_PS);
            nema_set_clip(0,0,width,height); nema_set_blend_blit(NEMA_BL_SRC);
            nema_blit(0,y);
            ++transitionLogicalCopies;
        }
        nema_bind_dst_tex(reinterpret_cast<uintptr_t>(rotatedBuffers[index]), width, height, NEMA_RGB565, width * 2);
        if (boardDisplayComposition.active) {
            // Layer rectangles on screen; the lower layer skips the band the
            // upper layer covers completely (sheet over a still page).
            Rect layer[2];
            for (unsigned n=0;n<2;n++) {
                layer[n]=Rect(n?c.incomingX:c.outgoingX,n?c.incomingY:c.outgoingY,width,height);
                if (n && !c.incomingClip.isEmpty()) layer[n] &= c.incomingClip;
                layer[n] &= Rect(0,0,width,height);
            }
            for (unsigned order=0;order<2;order++) {
                const unsigned n=c.outgoingOnTop?1-order:order;
                const int logicalX=n?c.incomingX:c.outgoingX;
                const int logicalY=n?c.incomingY:c.outgoingY;
                Rect logical(logicalX,logicalY,width,height);
                if (n) logical &= c.incomingClip;
                if (order==0 && (c.outgoingY || c.incomingY)) {
                    const Rect& top=layer[1-n];
                    if (top.width==(int)width && !top.isEmpty()) {
                        if (top.y<=logical.y && top.bottom()>logical.y) { const int b=logical.bottom(); logical.y=top.bottom(); logical.height=b-logical.y; }
                        else if (top.bottom()>=logical.bottom() && top.y<logical.bottom()) logical.height=top.y-logical.y;
                    }
                    if (logical.height<=0) continue;
                }
                else if (!n && !logicalX && !logicalY && !c.incomingX && !c.incomingY &&
                         !c.incomingClip.isEmpty()) {
                    // A wipe partitions the screen; avoid copying the outgoing
                    // pixels that will immediately be covered by the new view.
                    const Rect& revealed=boardDisplayComposition.incomingClip;
                    if (revealed.width==(int)width && revealed.height<(int)height) {
                        // Vertical wipe (menu sheet): partition along Y.
                        if (!revealed.y) { logical.y=revealed.bottom(); logical.height=height-logical.y; }
                        else logical.height=revealed.y;
                    }
                    else if (!revealed.x) { logical.x=revealed.right(); logical.width=width-logical.x; }
                    else logical.width=revealed.x;
                }
                logical &= Rect(0,0,width,height);
                if (logical.isEmpty()) continue;
                const int dx=width-logical.right(), dy=height-logical.bottom();
                const int textureX=logical.x-logicalX;
                const int textureY=logical.y-logicalY;
                const auto* pixels=(n?transitionLogicalSource:transitionOutgoingSource)+textureY*width+textureX;
                nema_bind_src_tex(reinterpret_cast<uintptr_t>(pixels),logical.width,logical.height,NEMA_RGB565,width*2,NEMA_FILTER_PS);
                nema_set_clip(0,0,width,height); nema_set_blend_blit(NEMA_BL_SRC);
                nema_blit_rotate(dx,dy,NEMA_ROT_180_CW);
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
        if(!boardDisplayComposition.active)logicalBuffersSynchronized=false;
        // Ordinary rendering remains the fallback if snapshot restoration was
        // interrupted; successful transitions already synchronized both buffers.
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
