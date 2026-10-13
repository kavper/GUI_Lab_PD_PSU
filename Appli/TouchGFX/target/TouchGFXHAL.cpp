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

using namespace touchgfx;

namespace
{
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint32_t animationBuffer[(800 * 480 * 2 + 3) / 4] LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
// Separate scanout buffers preserve TouchGFX's unrotated double buffering,
// including partial redraws, animation storage and framebuffer copies.
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint32_t rotatedBuffers[2][800 * 480 / 2] LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
uint16_t* volatile displayedLogicalBuffer = nullptr;
}

extern "C" {
volatile uint32_t BoardDisplayRotationFrames = 0;
}

void TouchGFXHAL::initialize()
{
    // Calling parent implementation of initialize().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.
    // Please note, HAL::initialize() must be called to initialize the framework.

    TouchGFXGeneratedHAL::initialize();
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

    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
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
    return TouchGFXGeneratedHAL::beginFrame();
}

void TouchGFXHAL::endFrame()
{
    if (frameBufferUpdatedThisFrame)
    {
        uint16_t* source = getClientFrameBuffer();
        const unsigned index = source == frameBuffer0 ? 0 : 1;
        InvalidateTextureCache();
        // Append rotation after drawing commands in the same GPU list. The
        // parent endFrame waits for completion before allowing a VSYNC swap.
        nema_bind_dst_tex(reinterpret_cast<uintptr_t>(rotatedBuffers[index]), 800, 480, NEMA_RGB565, 1600);
        nema_bind_src_tex(reinterpret_cast<uintptr_t>(source), 800, 480, NEMA_RGB565, 1600, NEMA_FILTER_PS);
        nema_set_clip(0, 0, 800, 480);
        nema_set_blend_blit(NEMA_BL_SRC);
        nema_blit_rotate(0, 0, NEMA_ROT_180_CW);
        submitExecBuffer();
        ++BoardDisplayRotationFrames;
    }
    TouchGFXGeneratedHAL::endFrame();
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
