
/**
 * =====================================================================
 * 用户配置区域
 * =====================================================================
 */

/** 配网方式 **/
// #define DISABLE_BLE_NET   // 禁用蓝牙配网，ESP32-C3 必须禁用一个
#define DISABLE_AP_NET // 禁用AP配网，ESP32-C3 必须禁用一个

/** 音频编解码方案 **/
#define CODEC_TYPE_BLAMP_I2S // 两路 I2S + MSM 数字麦克风 + MAX 数字功放   (ESP-AI-v1/v2/v3 开发板)
// #define CODEC_TYPE_ES8311_NS4150 // 一路 I2S + 8311 + NS4150               (ESP-AI-C3 开发板)
//  #define CODEC_TYPE_ES8311_ES7210 // 两路 I2S + 8311 + NS4150 + ES7210  (ESP-AI-v4 开发板)

/** 音频语言选择 **/
#define ESP_AI_LANGUAGE_ZH       // 中文音频文件
// #define ESP_AI_LANGUAGE_EN          // 英文音频文件
// #define ESP_AI_LANGUAGE_JA       // 日语音频文件（预留，待实现）



/**
 * 使用 4G 模块， 会自动删除wifi和配网相关代码以节省内存
 * 注意： 
 * 带屏幕的模块都使用扫码绑定设备
 * 没有屏幕需要考虑配网问题   test... ing...
 * 
*/
// #define USE_4G_MODULE