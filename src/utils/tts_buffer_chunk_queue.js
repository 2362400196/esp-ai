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
 
class TTS_buffer_chunk_queue {
    constructor(device_id) {
        this.device_id = device_id;
        this.queue = new Map([]);
        this.queueIndex = 0;
        this.stoped = true;
        this.runing = false;
    }
    push(index, func) {
        this.queue.set(index, func);
        !this.runing && this.run(this.queueIndex);
    }
    async run(awaitIndex) { 
        // 等待10s就算超时
        if (!this.queue.size || awaitIndex > 100) {
            this.stoped = true;
            this.runing = false;
            return;
        }
        this.runing = true;

        const tts_queue = this.queue.get(this.queueIndex);
        // 如果没有这个任务，说明任务还没就绪，等待 100ms 后继续监听
        if (!tts_queue) {
            return setTimeout(() => { this.run(awaitIndex += 1) }, 100);
        }
        this.queue.delete(this.queueIndex);
        await tts_queue();
        this.run();

        this.queueIndex += 1;
    }
    clear() {
        this.queueIndex = 0;
        this.runing = false;
        this.stoped = true;
        this.queue.clear();
    }
}
module.exports = TTS_buffer_chunk_queue;

