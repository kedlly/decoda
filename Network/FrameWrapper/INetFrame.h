#ifndef __I_NET_FRAME__
#define __I_NET_FRAME__


#define DEFAULT_FRAME_MARK					0x7e
#define DEFAULT_FRAME_ESCAPE				0x7d
#define DEFAULT_FRAME_ESC_MARK				0x02
#define DEFAULT_FRAME_ESC_ESCAPE			0x01


typedef unsigned char byte;

struct FrameDefinition
{
	byte Mark;						//标识位
	byte Escape;					//转义前导
	byte Esc_Mark;					//标识位信息值
	byte Esc_Escape;				//转义前导信息值
};

/*
* 数据帧格式
* |-----|--------------|-------------|------|
* |标识位|消息体(转义序列)|校验码(转义序列)|标识位 |
* |-----|--------------|-------------|------|
* 
* 标识位 一个字节 INET_FRAME_ID {FrameDefinition.Mark}
* 
* 若校验码、消息体中出现 INET_FRAME_ID，则要进行转义处理，转义
* 规则定义如下：
* 1 在 FrameDefinition 结构中定义
* 2 Mark     <--> Escape 后紧跟一个 Esc_Mark
* 3 Escape   <--> Escape 后紧跟一个 Esc_Escape
* 从而使得消息内容中不包含"标识位"，
* 
* 校验码 占用一个字节; 通过特定算法计算从消息开始到校验码前一个字节的校验值
* 
* 发送消息时：消息封装——>计算并填充校验码——>转义；
* 接收消息时：转义还原——>验证校验码——>解析消息
* 
*/

/// <summary>
/// 校验和计算方法
/// </summary>
typedef byte (*CalcCKS)(const byte* stream, int stream_length);

/// <summary>
/// 构建网络数据帧，并将数据写入"网络数据缓存" 中
/// 当buffer 为 nullptr时，返回构建网络帧所需的缓存长度
/// 返回值小于0表示缓存区buffer 长度不够 ,其绝对值为需补充长度
/// </summary>
/// <param name="message">原始消息</param>
/// <param name="msg_length">原始消息长度</param>
/// <param name="buffer">网络数据帧缓存</param>
/// <param name="buffer_size">网络数据帧缓存长度</param>
/// <returns>网络数据帧在buffer中的实际长度</returns>
int Build(const byte* message , int msg_length, byte* buffer, int buffer_size);

#define EC_SUCCEED                          0      //成功
#define EC_NOT_ENOUGH_MEMORY                1      //缓存不够
#define EC_INCORRECT_CKS                    2      //校验和错误
#define EC_ILLEGAL_FRAME					3      //帧信息错误

#define ECIF_OK								0
#define ECIF_HEAD                           (1 << 0)      //表示帧头数据错误
#define ECIF_TAIL                           (1 << 1)      //表示帧尾数据错误
#define ECIF_ILLEGAL_ESCAPE                 (1 << 2)      //表示帧数据中存在非法的转义数据
#define ECIF_SHORT_THEN_MIN_FRAME_LENGTH    (1 << 3)      //表示帧数据长度不满足最小帧长度需求

/// <summary>
/// 从"网络数据帧数据"中提取"消息"，并将消息写入消息缓存
/// 返回值 为 EC_SUCCEED 时            extData返回buffer中消息的实际字节数
///       为 EC_NOT_ENOUGH_MEMORY 时 表示缓存区buffer 长度不够, extData值为解析消息所需的最小长度
///       为 EC_INCORRECT_CKS 时表示 消息体验证校验和产生了错误 extData值为 第一字节为实际校验和，第二字节为当前cks计算函数得出的校验和
///		  为 EC_ILLEGAL_FRAME 时表示帧信息错误，
///                    extData值 按 ECIF_* 内容组合
/// </summary>
/// <param name="frame">网络数据帧</param>
/// <param name="frame_length">网络数据帧长度</param>
/// <param name="buffer">消息缓存</param>
/// <param name="buffer_size">消息缓存长度</param>
/// <param name="extData">补充数据</param>
/// <returns>错误码</returns>
int Parse(const byte* frame, int frame_length, byte* buffer, int buffer_size, int* extData);

/// <summary>
/// 设置 帧标识和帧转义字节定义
/// 返回原定义
/// 默认定义为 {INET_FRAME_ID, INET_FRAME_ESCAPE, INET_FRAME_ESC_ID, INET_FRAME_ESC_ESCAPE}
/// </summary>
/// <param name="ie">目标定义</param>
/// <returns>原定义</returns>
FrameDefinition SetIdentifierEscape(FrameDefinition ie);

/// <summary>
/// 设置 帧内容校验计算函数
/// 返回原函数
/// 校验计算函数指针为NULL时校验值强制为0，默认计算函数为计算消息体所有字节xor值
/// </summary>
/// <param name="target">目标函数</param>
/// <returns>原函数</returns>
CalcCKS SetCKS(CalcCKS target);


typedef void (*IFrameProc)(const byte* frame, const int frame_len);


/// <summary>
/// 从缓存中按顺序取出数据帧，并调用处理例程进行处理
/// </summary>
/// <param name="buffer">数据缓存</param>
/// <param name="buffer_size">数据缓存长度</param>
/// <param name="proc">数据帧处理例程</param>
/// <returns>buffer中包含的数据帧都被proc处理完毕后剩余的数据长度</returns>
int ReadFrames(const byte* buffer, int buffer_size, IFrameProc proc);

/// <summary>
/// 设置消息体最小尺寸(单位为字节数)
/// </summary>
/// <param name="bytes">字节数</param>
/// <returns>原尺寸数值</returns>
int SetMessageBaseSize(int bytes);

#endif //__I_NET_FRAME__