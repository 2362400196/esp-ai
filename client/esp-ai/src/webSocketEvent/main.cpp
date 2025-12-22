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
#include "main.h"

void getClockData(bool debug, const String &_ak, void (*onSetClockCb)(const String &cron, const String &text, const String &type))
{
    if (_ak)
    {
        DEBUG_PRINTLN(debug, ("[Info] -> 正在获取闹钟..."));
        HTTPClient get_clock_http;
        get_clock_http.begin(String(ESP_AI_SERVER) + "/alarm_timers/list_device");
        get_clock_http.addHeader("Content-Type", "application/json");
        JSONVar json_params;
        json_params["device_id"] = get_device_id();
        json_params["api_key"] = _ak;

        String send_data = JSON.stringify(json_params);
        int httpCode = get_clock_http.POST(send_data);
        if (httpCode > 0)
        {
            String payload = get_clock_http.getString();
            JSONVar parse_res = JSON.parse(payload);
            if (JSON.typeof(parse_res) == "undefined" || String(httpCode) != "200")
            {
                get_clock_http.end();
                Serial.println("[Error] -> 闹钟数据获取失败，错误码:" + String(httpCode));
            }

            if (parse_res.hasOwnProperty("success"))
            {
                bool success = (bool)parse_res["success"];
                String message = (const char *)parse_res["message"];
                if (success == false)
                {
                    get_clock_http.end();
                    Serial.println("[Error] -> 闹钟数据获取失败，错误原因：" + message);
                }
                else
                {
                    // 这里数据是 [ {  cron:"xx", id:"xxx" }, { cron:"xx", id:"xxx" }, .... ]
                    JSONVar data = parse_res["data"];
                    for (int i = 0; i < data.length(); i++)
                    {
                        String cron = (const char *)data[i]["cron"];
                        String text = (const char *)data[i]["desc"];
                        String clock_type = (const char *)data[i]["type"];
                        int sort = (int)data[i]["sort"];

                        if (clock_type == "1")
                        {
                            DEBUG_PRINT(debug, "[Info] -> 启动闹钟:");
                            DEBUG_PRINT(debug, sort);
                            DEBUG_PRINT(debug, " -> ");
                            DEBUG_PRINT(debug, cron);
                            DEBUG_PRINT(debug, " -> ");
                            DEBUG_PRINTLN(debug, text);

                            if (sort == 0)
                            {
                                clock_id_1 = Cron.create(cron.c_str(), clock_task_1, false);
                                clock_text_1 = text;
                            }
                            else if (sort == 1)
                            {
                                clock_id_2 = Cron.create(cron.c_str(), clock_task_2, false);
                                clock_text_2 = text;
                            }
                            else if (sort == 2)
                            {
                                clock_id_3 = Cron.create(cron.c_str(), clock_task_3, false);
                                clock_text_3 = text;
                            }

                            if (onSetClockCb != nullptr)
                            {
                                onSetClockCb(cron, text, "clock");
                            }
                        }
                        else if (clock_type == "2")
                        {
                            DEBUG_PRINT(debug, "[Info] -> 启动单次倒计时: ");
                            DEBUG_PRINT(debug, cron);
                            DEBUG_PRINT(debug, " -> ");
                            DEBUG_PRINTLN(debug, text);
 
                            timer_id_1 = Cron.create(cron.c_str(), timer_task_1, true);
                            timer_text_1 = text;
                            if (onSetClockCb != nullptr)
                            {
                                onSetClockCb(cron, text, "timer");
                            }
                        }
                        else if (clock_type == "3")
                        {
                            DEBUG_PRINT(debug, "[Info] -> 启动循环倒计时: ");
                            DEBUG_PRINT(debug, cron);
                            DEBUG_PRINT(debug, " -> ");
                            DEBUG_PRINTLN(debug, text);
 
                            timer_id_1 = Cron.create(cron.c_str(), timer_task_1_loop, false);
                            timer_text_1 = text;
                            if (onSetClockCb != nullptr)
                            {
                                onSetClockCb(cron, text, "timer");
                            }
                        }
                    }
                }
            }
            else
            {
                get_clock_http.end();
                Serial.println("[Error] -> 闹钟数据获取失败，请求服务失败！");
            }
        }
        else
        {
            Serial.println("[Error] -> 闹钟数据获取失败，请求服务失败！");
            get_clock_http.end();
        }
    }
}

void ESP_AI::webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{

    String api_key = getLocalData("api_key");
    String ext1 = getLocalData("ext1");
    String _ak = "";
    if (api_key != "")
    {
        _ak = api_key;
    }
    else
    {
        _ak = ext1;
    }

    switch (type)
    {
    case WStype_DISCONNECTED:

        if (esp_ai_ws_connected)
        {
            esp_ai_ws_connected = false;
            esp_ai_start_ed = false;
            esp_ai_session_id = "";
            asr_ing = false;
            Serial.print(F("[Info] -> ESP-AI 服务已断开："));
            Serial.println(length);

#if !defined(LITTLE_ROM)
            esp_ai_cache_audio_du.clear();
            esp_ai_cache_audio_greetings.clear();
#endif

            // 内置状态处理
            status_change("2");
            // 设备状态回调
            if (onNetStatusCb != nullptr)
            {
                esp_ai_net_status = "2";
                onNetStatusCb("2");
            }
            mp3_player_stop();
            connect_ws();
        }
        break;
    case WStype_CONNECTED:
    {
        Serial.println(F("[Info] -> ESP-AI 服务连接成功"));
        esp_ai_ws_connected = true;
        esp_ai_start_ed = false;
        esp_ai_session_id = "";
        asr_ing = false;
        xSemaphoreTake(audio_mutex, portMAX_DELAY);
        spk_ing = false;
        xSemaphoreGive(audio_mutex);

        if (xSemaphoreTake(esp_ai_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            JSONVar data_1;
            data_1["type"] = "play_audio_ws_conntceed";
            String sendData = JSON.stringify(data_1);
            esp_ai_webSocket.sendTXT(sendData);
            xSemaphoreGive(esp_ai_ws_mutex);
        } 
        // 内置状态处理
        status_change("3");
        // 设备状态回调
        if (onNetStatusCb != nullptr)
        {
            esp_ai_net_status = "3";
            onNetStatusCb("3");
        } 
        break;
    }
    case WStype_TEXT:
        if (strcmp((char *)payload, "session_end") == 0)
        {
            esp_ai_start_ed = false;
            esp_ai_session_id = "";
            esp_ai_tts_task_id = "";
            esp_ai_status = "3";
            asr_ing = false;
            xSemaphoreTake(audio_mutex, portMAX_DELAY);
            spk_ing = false;
            xSemaphoreGive(audio_mutex);
        }
        else
        {
            JSONVar parseRes = JSON.parse((char *)payload);
            if (JSON.typeof(parseRes) == "undefined")
            {
                return;
            }
            if (debug)
            {
                Serial.print(F("[Info] -> Received Text: "));
                Serial.println((char *)payload);
            }

            if (parseRes.hasOwnProperty("type"))
            {
                String type = (const char *)parseRes["type"];
                String command_id = "";
                String data = "";
                if (parseRes.hasOwnProperty("command_id"))
                {
                    command_id = (const char *)parseRes["command_id"];
                }
                if (parseRes.hasOwnProperty("data"))
                {
                    data = (const char *)parseRes["data"];
                }

                if (type == "stc_time")
                {
                    if (xSemaphoreTake(esp_ai_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        String stc_time = parseRes["stc_time"];
                        JSONVar data_delayed;
                        data_delayed["type"] = "cts_time";
                        data_delayed["stc_time"] = stc_time;
                        String sendData = JSON.stringify(data_delayed);
                        esp_ai_webSocket.sendTXT(sendData);
                        xSemaphoreGive(esp_ai_ws_mutex);
                    }
                }

                else if (type == "net_delay")
                {
                    int net_delay = parseRes["net_delay"];
                    DEBUG_PRINTLN(debug, "[Info] -> 网络延时：" + String(net_delay) + "ms");

                    // 网络延迟过高时给出提示
                    // ing...
                    if (onNetDelayCb != nullptr)
                    {
                        onNetDelayCb(net_delay);
                    }

                    uint64_t now = (uint64_t)(double)parseRes["now"];
                    // 转换为秒和微秒
                    time_t seconds = now / 1000;
                    suseconds_t useconds = (now % 1000) * 1000;

                    // 设置 UTC 时间
                    timeval tv;
                    tv.tv_sec = seconds;
                    tv.tv_usec = useconds;
                    settimeofday(&tv, nullptr);

                    // 设置上海时区 (UTC+8)
                    setenv("TZ", "CST-8", 1); // 中国标准时间，东八区
                    tzset();

                    // 恢复闹钟
                    getClockData(debug, _ak, onSetClockCb);
                }

                // user command
                else if (type == "instruct")
                {

                    if (command_id == "cancel_timer")
                    {
                        DEBUG_PRINTLN(debug, "[Info] -> 取消倒计时");
                        Cron.free(timer_id_1);
                        timer_id_1 = dtINVALID_ALARM_ID;
                        timer_text_1 = "";
                        // 请求接口, 两种情况都需要清除
                        set_clock(_ak, "0", "0", "2", "del");
                        set_clock(_ak, "0", "0", "3", "del");

                        if (clock_text_1 == "" && clock_text_2 == "" && clock_text_3 == "" && timer_text_1 == "")
                        {
                            if (onClearClockCb != nullptr)
                                onClearClockCb();
                        }
                        return;
                    }
                    if (command_id == "cancel_clock")
                    {
                        DEBUG_PRINTLN(debug, "[Info] -> 取消闹钟");
                        Cron.free(clock_id_1);
                        clock_id_1 = dtINVALID_ALARM_ID;
                        clock_text_1 = "";
                        // 请求接口
                        set_clock(_ak, "0", "0", "1", "del");

                        if (clock_text_1 == "" && clock_text_2 == "" && clock_text_3 == "" && timer_text_1 == "")
                        {
                            if (onClearClockCb != nullptr)
                                onClearClockCb();
                        }
                        return;
                    }

                    if (onEventCb != nullptr)
                    {
                        onEventCb(command_id, data);
                    }
                    // 应答帧
                    if (xSemaphoreTake(esp_ai_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        esp_ai_webSocket.sendTXT("{ \"type\":\"instruct_ack\"}");
                        xSemaphoreGive(esp_ai_ws_mutex);
                    }
                }
                else if (type == "cron_task")
                {

                    String cron = (const char *)parseRes["cron"];
                    String text = (const char *)parseRes["text"];
                    String clock_type = (const char *)parseRes["clock_type"]; // 闹钟类型，'1' 定时器闹钟 '2' 单次倒计时 '3' 循环倒计时

                    if (clock_type == "1")
                    {
                        DEBUG_PRINT(debug, "[Info] -> 收到闹钟: ");
                        DEBUG_PRINT(debug, cron);
                        DEBUG_PRINT(debug, " -> ");
                        DEBUG_PRINTLN(debug, text);

                        clock_id_1 = Cron.create(cron.c_str(), clock_task_1, false);
                        clock_text_1 = text;
                        set_clock(_ak, text, cron, clock_type, "set");

                        if (onSetClockCb != nullptr)
                        {
                            onSetClockCb(cron, text, "clock");
                        }
                    }
                    else if (clock_type == "2")
                    {
                        DEBUG_PRINT(debug, "[Info] -> 收到单次倒计时: ");
                        DEBUG_PRINT(debug, cron);
                        DEBUG_PRINT(debug, " -> ");
                        DEBUG_PRINTLN(debug, text);

                        Cron.free(timer_id_1);
                        timer_id_1 = dtINVALID_ALARM_ID;
 
                        timer_id_1 = Cron.create(cron.c_str(), timer_task_1, true);
                        timer_text_1 = text;
                        set_clock(_ak, text, cron, clock_type, "set");

                        if (onSetClockCb != nullptr)
                        {
                            onSetClockCb(cron, text, "timer");
                        }
                    }
                    else if (clock_type == "3")
                    {
                        DEBUG_PRINT(debug, "[Info] -> 收到循环倒计时: ");
                        DEBUG_PRINT(debug, cron);
                        DEBUG_PRINT(debug, " -> ");
                        DEBUG_PRINTLN(debug, text);

                        Cron.free(timer_id_1);
                        timer_id_1 = dtINVALID_ALARM_ID;
 
                        timer_id_1 = Cron.create(cron.c_str(), timer_task_1_loop, false);
                        timer_text_1 = text;
                        set_clock(_ak, text, cron, clock_type, "set");

                        if (onSetClockCb != nullptr)
                        {
                            onSetClockCb(cron, text, "timer");
                        }
                    }
                }

                // tts task log
                else if (type == "play_audio")
                {
                    // 上报音频时
                    if (esp_ai_start_send_audio)
                    {
                        esp_ai_tts_task_id = "";
                        return;
                    }

                    open_spk();
                    xSemaphoreTake(audio_mutex, portMAX_DELAY);
                    spk_ing = true;
                    xSemaphoreGive(audio_mutex);
                    esp_ai_tts_task_id = (const char *)parseRes["tts_task_id"];
                    DEBUG_PRINTLN(debug, "[TTS] -> TTS 任务：" + esp_ai_tts_task_id);
                    if (esp_ai_tts_task_id == "play_music" && onEventCb != nullptr)
                    {
                        onEventCb("play_music", "");
                    }
                }
                else if (type == "session_start")
                {
                    esp_ai_session_id = (const char *)parseRes["session_id"];
                    xSemaphoreTake(audio_mutex, portMAX_DELAY);
                    spk_ing = true;
                    xSemaphoreGive(audio_mutex);
                }

                else if (type == "session_stop")
                {
                    // 应答帧
                    if (xSemaphoreTake(esp_ai_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        String sid = (const char *)parseRes["session_id"];
                        esp_ai_webSocket.sendTXT("{ \"type\":\"session_stop_ack\", \"session_id\": \"" + sid + "\"}");
                        xSemaphoreGive(esp_ai_ws_mutex);
                    }

                    // 上报音频时
                    if (esp_ai_start_send_audio)
                    {
                        esp_ai_tts_task_id = "";
                        return;
                    }

                    /**
                     * 这里仅仅是停止，并不能结束录音
                     * 还有这里不能结束播放，可能是 .tts 的播放音，除非用户指定
                     */
                    esp_ai_session_id = "";
                    if (data == "1")
                    {
                        asr_ing = false;
                        mp3_player_stop();
                    }
                }

                else if (type == "auth_fail")
                {
                    String message = (const char *)parseRes["message"];
                    String code = (const char *)parseRes["code"];
                    Serial.print("\n[Error] -> 连接服务失败，code: ");
                    Serial.print(code);
                    Serial.println(" message: ");
                    Serial.println(message);
                    Serial.println(F("[Error] -> 请检测服务器配置中是否配置了鉴权参数。"));

                    handel_error(code);

                    if (onErrorCb != nullptr)
                    {
                        onErrorCb("002", "auth", message);
                    }
                }
                else if (type == "error")
                {
                    String at_pos = (const char *)parseRes["at"];
                    String message = (const char *)parseRes["message"];
                    String code = (const char *)parseRes["code"];
                    Serial.println("[Error] -> 服务错误：" + at_pos + " " + code + " " + message);
                    handel_error(code);

                    if (code == "102" || code == "101" || code == "100")
                    {
                        esp_ai_session_id = "";
                        asr_ing = false;
                    }
                    if (onErrorCb != nullptr)
                    {
                        onErrorCb(code, at_pos, message);
                    }
                }

                else if (type == "session_status")
                {
                    String status = (const char *)parseRes["status"];
                    if (status == "iat_end")
                    {

                        esp_ai_start_ed = false;
                        esp_ai_start_send_audio = false;
                        asr_ing = false;
                        open_spk();
                        xSemaphoreTake(audio_mutex, portMAX_DELAY);
                        spk_ing = true;
                        xSemaphoreGive(audio_mutex);
                    }
                    else if (status == "iat_start")
                    {
                        send_start_time = 0;
                        // 开始发送音频时，先等话说完
                        wait_mp3_player_done();
                        // 正在说话时就不要继续推理了，否则会误唤醒
                        esp_ai_start_ed = true;
                        // 开始发送音频时，将缓冲区中的数据发送出去
                        esp_ai_start_send_audio = true;
                    }
                    // 内置状态处理
                    status_change(status);
                    if (onSessionStatusCb != nullptr)
                    {
                        onSessionStatusCb(status);
                    }
                }
                else if (type == "set_wifi_config")
                {

                    if (xSemaphoreTake(esp_ai_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {

                        JSONVar JSON_data = parseRes["configs"];
                        bool is_ok = setWifiConfig(JSON_data);

                        JSONVar set_wifi_config_res;
                        set_wifi_config_res["type"] = "set_wifi_config_res";
                        set_wifi_config_res["success"] = is_ok;
                        String sendData = JSON.stringify(data);
                        DEBUG_PRINTLN(debug, F("[TTS] -> 发送设置WiFi参数结果到服务端"));
                        esp_ai_webSocket.sendTXT(sendData);
                        xSemaphoreGive(esp_ai_ws_mutex);
                    }
                }

                else if (type == "restart")
                {
                    ESP.restart();
                }
                else if (type == "clear_cache")
                {
#if !defined(LITTLE_ROM)
                    if (!esp_ai_cache_audio_du.empty())
                    {
                        esp_ai_cache_audio_du.clear();
                    }
                    if (!esp_ai_cache_audio_greetings.empty())
                    {
                        esp_ai_cache_audio_greetings.clear();
                    }
#endif
                    // 应答帧
                    if (xSemaphoreTake(esp_ai_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        esp_ai_webSocket.sendTXT("{ \"type\":\"clear_cache_ack\"}");
                        xSemaphoreGive(esp_ai_ws_mutex);
                    }
                }
                else if (type == "set_local_data")
                {
                    String field = (const char *)parseRes["field"];
                    String value = (const char *)parseRes["value"];
                    set_local_data(field, value);
                }
                else if (type == "log")
                {
                    String data = (const char *)parseRes["data"];
                }
                else if (type == "sever-close")
                {
                    DEBUG_PRINT(debug, F("[Error] 服务端主动断开，尝试重新连接。"));
                    ESP.restart();
                }

                else if (type == "hardware-fns")
                {
                    int pin = (int)parseRes["pin"];
                    String fn_name = (const char *)parseRes["fn_name"];
                    String str_val = (const char *)parseRes["str_val"];
                    int num_val = (int)parseRes["num_val"];

                    // 设置引脚模式
                    if (fn_name == "pinMode")
                    {
                        str_val == "OUTPUT" && (pinMode(pin, OUTPUT), true);
                        str_val == "INPUT" && (pinMode(pin, INPUT), true);
                        str_val == "INPUT_PULLUP" && (pinMode(pin, INPUT_PULLUP), true);
                        str_val == "INPUT_PULLDOWN" && (pinMode(pin, INPUT_PULLDOWN), true);

                        // LEDC
                        if (str_val == "LEDC")
                        {
                            // LEDC 通道, 取值 0 ~ 15
                            int channel = 0;
                            if (parseRes.hasOwnProperty("channel"))
                            {
                                channel = (int)parseRes["channel"];
                            }
                            // 定义 PWM 频率，舵机通常使用 50Hz
                            int freq = 50;
                            if (parseRes.hasOwnProperty("freq"))
                            {
                                freq = (int)parseRes["freq"];
                            }
                            // 定义 PWM 分辨率
                            int resolution = 10;
                            if (parseRes.hasOwnProperty("resolution"))
                            {
                                resolution = (int)parseRes["resolution"];
                            }

                            // 初始化 LEDC 通道
                            ledcSetup(channel, freq, resolution);
                            // 将 LEDC 通道绑定到指定引脚
                            ledcAttachPin(pin, channel);
                        }
                    }
                    else if (fn_name == "digitalWrite")
                    {
                        str_val == "HIGH" && (digitalWrite(pin, HIGH), true);
                        str_val == "LOW" && (digitalWrite(pin, LOW), true);
                    }
                    else if (fn_name == "digitalRead")
                    {
                        digital_read_pins.push_back(pin);
                    }
                    else if (fn_name == "analogWrite")
                    {
                        analogWrite(pin, num_val);
                    }
                    else if (fn_name == "analogRead")
                    {
                        analog_read_pins.push_back(pin);
                    }
                    // 舵机驱动
                    else if (fn_name == "ledcWrite")
                    {
                        int channel = 0;
                        if (parseRes.hasOwnProperty("channel"))
                        {
                            channel = (int)parseRes["channel"];
                        }
                        int deg = (int)parseRes["deg"];
                        ledcWrite(channel, angleToDutyCycle(deg));
                    }
                }
                // 情绪监听
                else if (type == "emotion")
                {
                    if (onEmotionCb != nullptr)
                    {
                        onEmotionCb(data);
                    }
                }
            }
        }

        break;
    case WStype_BIN:
    {

        if (length < 6)
        {
            Serial.print(F("[Error] -> 数据帧长度小于6字节: "));
            Serial.println(length);
            return;
        }

        // 会话ID
        char session_id_string[5];
        memcpy(session_id_string, payload, 4);
        session_id_string[4] = '\0';
        String sid = String(session_id_string);

        // 会话状态
        char session_status_string[3];
        memcpy(session_status_string, payload + 4, 2);
        session_status_string[2] = '\0';
        esp_ai_session_status = String(session_status_string);

        // test...
        Serial.print("内容长度：");
        Serial.print(length);
        Serial.print("  会话ID：");
        Serial.print(sid);
        Serial.print("  会话状态：");
        Serial.print(esp_ai_session_status);
        Serial.println("");

        // 提取音频数据
        uint8_t *audioData = payload + 6;
        size_t audioLength = length - 6;

        if (sid == SID_TONE_CACHE)
        {
#if !defined(LITTLE_ROM)
            esp_ai_cache_audio_du.insert(esp_ai_cache_audio_du.end(), audioData, audioData + audioLength);
#endif
        }
        else if (sid == SID_WAKEUP_REP_CACHE)
        {
#if !defined(LITTLE_ROM)
            esp_ai_cache_audio_greetings.insert(esp_ai_cache_audio_greetings.end(), audioData, audioData + audioLength);
#endif
        }
        else
        {
            // 会话ID 不正确的分组数据直接抛弃  SID_WAKEUP_REP_CACHE
            if (session_id_string && sid != SID_TONE && sid != SID_CONNECTED_SERVER && sid != SID_TTS_FN && sid != esp_ai_session_id)
                return;
            if (sid == SID_CONNECTED_SERVER && esp_ai_played_connected)
                return;

            // Serial.print("  写入长度：");
            // Serial.print(audioLength);
            // Serial.print("    正在播放长度：");
            // Serial.println(esp_ai_spk_queue.available());
            mp3_player_write(audioData, audioLength);
        }

        if (esp_ai_session_status == SID_TTS_END_RESTART)
        {

            esp_ai_tts_task_id = "";
            // 内置状态处理
            status_change("tts_real_end");
            if (esp_ai_session_id != "")
            {
                wait_mp3_player_done();
                if (xSemaphoreTake(esp_ai_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    esp_ai_webSocket.sendTXT("{ \"type\":\"client_out_audio_over\", \"session_id\": \"" + sid + "\",  \"session_status\": \"" + esp_ai_session_status + "\", \"tts_task_id\": \"" + esp_ai_tts_task_id + "\" }");
                    xSemaphoreGive(esp_ai_ws_mutex);
                }

                if (onSessionStatusCb != nullptr)
                {
                    onSessionStatusCb("tts_real_end");
                }

                if (!esp_ai_is_listen_model)
                {
                    // tts发送完毕，需要重新开启录音
                    DEBUG_PRINTLN(debug, F("[TTS] -> TTS 数据全部接收完毕，需继续对话。"));
                    asr_ing = false;
                    xSemaphoreTake(audio_mutex, portMAX_DELAY);
                    spk_ing = false;
                    xSemaphoreGive(audio_mutex);
                    wakeUp("continue");
                }
            }
        }
        else if (esp_ai_session_status == SID_TTS_END)
        {

            // 服务连接成功播放完毕
            bool is_first_connect = esp_ai_played_connected == false && sid == SID_CONNECTED_SERVER && esp_ai_session_status == SID_TTS_END;
            if (is_first_connect)
            {
                esp_ai_played_connected = true;
            }

            wait_mp3_player_done();
            if (xSemaphoreTake(esp_ai_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                esp_ai_webSocket.sendTXT("{ \"type\":\"client_out_audio_over\", \"session_id\": \"" + sid + "\", \"session_status\": \"" + esp_ai_session_status + "\", \"tts_task_id\": \"" + esp_ai_tts_task_id + "\"}");
                xSemaphoreGive(esp_ai_ws_mutex);
            }

            esp_ai_tts_task_id = "";
            // 内置状态处理
            status_change("tts_real_end");

            if (onSessionStatusCb != nullptr)
            {
                onSessionStatusCb("tts_real_end");
                onSessionStatusCb("session_end");
            }
            DEBUG_PRINT(debug, F("[TTS] -> TTS 数据全部接收完毕，无需继续对话"));
            esp_ai_start_ed = false;

            // 服务连接成功播放完毕
            if (is_first_connect && onReadyCb != nullptr)
            {
                wait_mp3_player_done();
                vTaskDelay(300 / portTICK_PERIOD_MS);
                onReadyCb();
            }
            ready_ed = true;

            return;
        }
        else if (esp_ai_session_status == SID_TTS_CHUNK_END)
        {
            esp_ai_tts_task_id = "";
        }
        break;
    }
    // case WStype_PING:
    //     Serial.println("Ping");
    //     break;
    // case WStype_PONG:
    //     Serial.println("Pong");
    //     break;
    case WStype_ERROR:
        Serial.println(F("[Error] 服务 WebSocket 连接错误"));
        break;
    }
}
