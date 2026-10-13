/* New PCB panel: PC6/PC7 + PE2 (V), PD12/PD13 + PD14 (I), PE6 (OUTPUT).
 * PA5/TIM2_CH1 drives the passive 4kHz piezo through R619. */
#include "board_panel.h"
#include "panel_input.h"
#include "psu_app.h"
#include "main.h"
#include "cmsis_os2.h"
static PanelInput input;
static osMessageQueueId_t events;
static uint32_t tone_end, last_tone;
static uint8_t tone_active;
static volatile uint8_t sound_volume = 16, sound_tone = 0, sound_feedback = 3;
static volatile uint32_t sound_request;
/* PWM frequency, duty ticks, tone starts. Idle CCR1=0 is expected. */
volatile uint32_t BoardBuzzerDiagnostics[3];
void BoardBuzzer_Configure(uint8_t volume, uint8_t tone, uint8_t feedback)
{
    sound_volume = volume > 100 ? 100 : volume;
    sound_tone = tone % 3; sound_feedback = feedback & 3;
}
void BoardBuzzer_Request(uint32_t duration_ms)
{
    sound_request = duration_ms > 500 ? 500 : duration_ms;
}
static TIM_HandleTypeDef voltage_timer, current_timer;
/* V turns, I turns, V step clicks, I step clicks, short OUTPUT, hold OUTPUT, queue drops. */
volatile uint32_t BoardPanelDiagnostics[7];
static uint8_t keys(void)
{
    uint32_t d = GPIOD->IDR, e = GPIOE->IDR;
    return (!(e & GPIO_PIN_2)) | ((!(d & GPIO_PIN_14)) << 1) | ((!(e & GPIO_PIN_6)) << 2);
}
static void encoder_init(TIM_HandleTypeDef *h, TIM_TypeDef *timer)
{
    TIM_Encoder_InitTypeDef config = {0};
    h->Instance = timer;
    h->Init.Prescaler = 0; h->Init.CounterMode = TIM_COUNTERMODE_UP;
    h->Init.Period = 65535; h->Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
    h->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    config.EncoderMode = TIM_ENCODERMODE_TI12;
    config.IC1Polarity = config.IC2Polarity = TIM_ICPOLARITY_RISING;
    config.IC1Selection = config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    config.IC1Prescaler = config.IC2Prescaler = TIM_ICPSC_DIV1;
    config.IC1Filter = config.IC2Filter = 15;
    if (HAL_TIM_Encoder_Init(h, &config) != HAL_OK ||
        HAL_TIM_Encoder_Start(h, TIM_CHANNEL_ALL) != HAL_OK) Error_Handler();
}
static void start_tone(uint32_t now, uint32_t duration)
{
    if (!sound_volume || !duration) return;
    const uint32_t frequencies[] = {4000, 2000, 1000};
    const uint32_t hz = frequencies[sound_tone];
    const uint32_t period = 1000000U / hz;
    const uint32_t duty = period * sound_volume / 200U;
    TIM2->ARR = period - 1; TIM2->CCR1 = duty;
    TIM2->EGR = TIM_EGR_UG;
    last_tone = now; tone_end = now + duration; tone_active = 1;
    BoardBuzzerDiagnostics[0] = hz; BoardBuzzerDiagnostics[1] = duty;
    ++BoardBuzzerDiagnostics[2];
}
static void quiet_tone(uint32_t now, uint32_t duration)
{
    if (!(sound_feedback & 2) || tone_active || (uint32_t)(now-last_tone) < 50) return;
    start_tone(now, duration);
}
static void enqueue(uint8_t kind, int8_t delta)
{
    BoardPanelEvent e = {kind, delta};
    if (osMessageQueuePut(events, &e, 0, 0) != osOK) ++BoardPanelDiagnostics[6];
}
void BoardPanel_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE(); __HAL_RCC_GPIOE_CLK_ENABLE();
    g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL; g.Alternate = GPIO_AF2_TIM3;
    g.Pin = GPIO_PIN_6 | GPIO_PIN_7; HAL_GPIO_Init(GPIOC, &g);
    g.Alternate = GPIO_AF2_TIM4;
    g.Pin = GPIO_PIN_12 | GPIO_PIN_13; HAL_GPIO_Init(GPIOD, &g);
    g.Mode = GPIO_MODE_INPUT;
    g.Pin = GPIO_PIN_14; HAL_GPIO_Init(GPIOD, &g);
    g.Pin = GPIO_PIN_2; HAL_GPIO_Init(GPIOE, &g);
    __HAL_RCC_TIM3_CLK_ENABLE(); __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_TIM3_FORCE_RESET(); __HAL_RCC_TIM3_RELEASE_RESET();
    __HAL_RCC_TIM4_FORCE_RESET(); __HAL_RCC_TIM4_RELEASE_RESET();
    encoder_init(&voltage_timer, TIM3); encoder_init(&current_timer, TIM4);
    g.Pin = GPIO_PIN_5; g.Mode = GPIO_MODE_AF_PP; g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF1_TIM2; HAL_GPIO_Init(GPIOA, &g);
    __HAL_RCC_TIM2_CLK_ENABLE(); __HAL_RCC_TIM2_FORCE_RESET(); __HAL_RCC_TIM2_RELEASE_RESET();
    RCC_ClkInitTypeDef clocks; uint32_t latency;
    HAL_RCC_GetClockConfig(&clocks, &latency);
    uint32_t hz = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_APB1_DIV1)
        hz *= clocks.APB1CLKDivider == RCC_APB1_DIV2 || __HAL_RCC_GET_TIMCLKPRESCALER() == RCC_TIMPRES_DISABLE ? 2 : 4;
    TIM2->PSC = hz / 1000000U - 1; TIM2->ARR = 249; TIM2->CCR1 = 0;
    TIM2->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE;
    TIM2->CCER = TIM_CCER_CC1E; TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
    panel_input_init(&input, TIM3->CNT, TIM4->CNT, keys(), HAL_GetTick());
    events = osMessageQueueNew(32, sizeof(BoardPanelEvent), NULL);
    if (!events) Error_Handler();
}
void BoardPanel_Poll(uint32_t now)
{
    if (tone_active && (!sound_volume || (int32_t)(now-tone_end) >= 0)) { TIM2->CCR1 = 0; tone_active = 0; }
    /* H7RS shareable AXI SRAM does not accept the exclusive LDREX exchange.
     * A bounded interrupt critical section also protects against task switches. */
    const uint32_t interrupt_mask = __get_PRIMASK();
    __disable_irq();
    uint32_t request = sound_request; sound_request = 0;
    __set_PRIMASK(interrupt_mask);
    if (request && (request > 30 || (sound_feedback & 1))) start_tone(now, request);
    PanelActions a = panel_input_update(&input, TIM3->CNT, TIM4->CNT, keys(), now);
    if (a.voltage) { BoardPanelDiagnostics[0] += a.voltage < 0 ? -a.voltage : a.voltage; enqueue(BOARD_VOLTAGE, a.voltage); }
    if (a.current) { BoardPanelDiagnostics[1] += a.current < 0 ? -a.current : a.current; enqueue(BOARD_CURRENT, a.current); }
    if (a.voltage_step) { ++BoardPanelDiagnostics[2]; enqueue(BOARD_VOLTAGE_STEP, 0); }
    if (a.current_step) { ++BoardPanelDiagnostics[3]; enqueue(BOARD_CURRENT_STEP, 0); }
    if (a.output == 1) {
        PsuSnapshot s; psu_snapshot(&s);
        ++BoardPanelDiagnostics[4];
        if (s.output_requested || s.output_confirmed || s.psu_running) (void)psu_app_shutdown();
        else (void)psu_app_set_output(1, PSU_SRC_BUTTON);
    } else if (a.output == 2) {
        ++BoardPanelDiagnostics[5]; (void)psu_app_power_shutdown_from(PSU_SRC_BUTTON);
    }
    if (a.output) quiet_tone(now, a.output == 2 ? 40 : 20);
    else if (a.voltage_step || a.current_step) quiet_tone(now, 15);
    else if (a.voltage || a.current) quiet_tone(now, 8);
}
int BoardPanel_Take(BoardPanelEvent *event)
{
    return events && osMessageQueueGet(events, event, NULL, 0) == osOK;
}
