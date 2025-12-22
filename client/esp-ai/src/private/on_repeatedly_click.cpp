/*
 * MIT License
 *
 * Copyright (c) 2025-至今 小明IO
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @author 小明IO
 * @email  1746809408@qq.com
 * @github https://github.com/wangzongming/esp-ai
 * @websit https://espai.fun
 */
#include "on_repeatedly_click.h"

StaticTask_t onRepeatedlyClickTaskBuffer;
StackType_t onRepeatedlyClickTaskStack[ON_REPEATEDLY_CLICK_TASK_SIZE];

void on_repeatedly_click_task_static(void *arg)
{
    OnRepeatedlyClickContext *ctx = static_cast<OnRepeatedlyClickContext *>(arg);
    if (!ctx)
    {
        Serial.println(F("[Error] OnRepeatedlyClickContext ctx is null!"));
        vTaskDelete(NULL);
        return;
    }

    long debounce_time = 0;
    const int debounce_delay = 250;
    int prev_state = (*(ctx->power) == "high") ? 0 : 1;
    int prev_state_boot = 1;

    long last_click_time = 0;
    int click_count = 0;
    const int max_click_delay = 1000;
    bool reset_ing = false;

    while (true)
    {
        if (reset_ing)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            return;
        }
        int reading = digitalRead(*(ctx->pin));

        long now = millis();
        int target_val = (*(ctx->power) == "high") ? 1 : 0;

        bool boot_clicked = false;
        int reading_boot = 1;
#if defined(IS_BOOT_RESET)
        reading_boot = digitalRead(BOOT_BUTTON_GPIO);
        if (reading_boot == 0)
        {
            boot_clicked = true;
        }
#endif

        if ((reading == target_val) || boot_clicked)
        {
            if ((now - debounce_time) > debounce_delay)
            {
                debounce_time = now;

                if ((prev_state != reading) || (prev_state_boot != reading_boot))
                {
                    prev_state = reading;
                    prev_state_boot = reading_boot;

                    if ((last_click_time == 0) || ((now - last_click_time) <= max_click_delay))
                    {
                        last_click_time = now;
                        click_count++;

                        if (click_count == 5)
                        {
                            reset_ing = true;
                            click_count = 0;
                            last_click_time = 0;
                            // 清除会话 ID
                            *(const_cast<String *>(ctx->esp_ai_session_id)) = "";

                            DEBUG_PRINTLN(*(ctx->debug), F("触发重置设置"));

                            if (ctx->on_repeatedly_click_cb != nullptr)
                                ctx->on_repeatedly_click_cb();

                            if (ctx->clear_data != nullptr)
                                ctx->clear_data();

                            // 这个一定需要放到后面
                            ctx->play_builtin_audio(hui_fu_chu_chang, hui_fu_chu_chang_len);
                            ctx->wait_mp3_player_done();

                            vTaskDelay(100 / portTICK_PERIOD_MS);
                            ESP.restart(); // 重启设备
                        }
                    }
                }
            }
        }
        else
        {
            prev_state = reading;
            prev_state_boot = reading_boot;
            if (click_count != 0 && ((now - last_click_time) >= max_click_delay))
            {
                click_count = 0;
                last_click_time = 0;
            }
        }

        vTaskDelay(30 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}