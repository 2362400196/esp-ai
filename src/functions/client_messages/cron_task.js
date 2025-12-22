/**
 * Copyright (c) 2024 小明IO
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Commercial use of this software requires prior written authorization from the Licensor.
 * 请注意：将 ESP-AI 代码用于商业用途需要事先获得许可方的授权。
 * 删除与修改版权属于侵权行为，请尊重作者版权，避免产生不必要的纠纷。
 * 
 * @author 小明IO   
 * @email  1746809408@qq.com
 * @github https://github.com/wangzongming/esp-ai
 * @websit https://espai.fun
 */

/**
 * 客户端连接成功
*/
const { t_info, error: errorLog } = require("../../utils/log");
const axios = require('axios');

async function fn({ device_id, text }) {
    if (!G_devices.get(device_id)) return;  
    try { 
        const { api_key: g_api_key, ai_server } = G_config;
        const { ws: ws_client } = G_devices.get(device_id);

        await G_Instance.stop(device_id, "设备断开服务时", true);
        t_info(`设备 ${device_id} 启动定时任务 ->  ${text}`)
        // 定时、闹钟任务匹配 
        const response = await axios.post(`${ai_server}/ai_api/get_text_alert`, {
            "api_key": g_api_key,
            "text": text
        }, { headers: { 'Content-Type': 'application/json' } });
        const { success, message, data } = response.data;

        if (!success) {
            errorLog(`cron_task 请求错误： ${message}`) 
        } else if (data) {   
            ws_client && ws_client.send(JSON.stringify({ type: "session_stop", data: "1", session_id:"" }));
            await G_Instance.tts(device_id, data);
        } 
    } catch (err) {
        console.log(err);
        errorLog(`cron_task 消息错误： ${err}`)
    }
}

module.exports = fn