
#include "INetFrame.h"
#include <assert.h>
#include <memory>

static FrameDefinition curFD =
{
	  DEFAULT_FRAME_MARK
	, DEFAULT_FRAME_ESCAPE
	, DEFAULT_FRAME_ESC_MARK
	, DEFAULT_FRAME_ESC_ESCAPE
};


							
#define FRAME_MARK			curFD.Mark
#define FRAME_ESCAPE		curFD.Escape
#define FRAME_ESC_MARK		curFD.Esc_Mark
#define FRAME_ESC_ESCAPE	curFD.Esc_Escape



static int min_message_length = 1;


#define MIN_FRAME_LEN int( sizeof(FRAME_MARK) + min_message_length + sizeof(byte) + sizeof(FRAME_MARK))


byte _calcCKS(const byte* stream, int stream_length)
{
	byte cks = 0;
	if (stream_length > 0 && stream != nullptr)
	{
		cks = stream[0];
		for (int i = 1; i < stream_length; ++i)
		{
			cks ^= stream[i];
		}
	}
	return cks;
}

static CalcCKS cks_current = _calcCKS;


FrameDefinition setIdentifierEscape(FrameDefinition ie)
{
	FrameDefinition old = curFD;
	curFD = ie;
	return old;
}

CalcCKS setCKS(CalcCKS target)
{
	CalcCKS old = cks_current;
	cks_current = target;
	return old;
}



int Build(const byte* message, int msg_length, byte* buffer, int buffer_size)
{
	int EscapeMessage(const byte * message, int msg_length, byte * buffer, int buffer_size);
	int GetEscapeMessageSize(const byte * message, int msg_length);
	int escape_msg_length = GetEscapeMessageSize(message, msg_length);
	if (escape_msg_length == 0)
	{
		return 0;
	}
	byte cks = cks_current(message, msg_length);
	//desireLength = sizeof(HEAD) + escape_msg_length + sizeof(escape_CKS) + sizeof(END)
	int ret = 1 + escape_msg_length + 0 + 1;
	int escape_cks_len = cks == FRAME_ESCAPE || cks == FRAME_MARK ? 2 : 1;
	ret += escape_cks_len;
	
	if (buffer != nullptr)
	{
		if (ret <= buffer_size)
		{
			byte* content = buffer;
			content[0] = FRAME_MARK;
			content += 1; //sizeof(HEAD)
			//byte* escape_msg_begin = content;
			int emsg_len = EscapeMessage(message, msg_length, content, escape_msg_length);
			assert(emsg_len == escape_msg_length);
			content += emsg_len;
			int esc_cks_len = EscapeMessage(&cks, 1, content, escape_cks_len);
			assert(escape_cks_len == esc_cks_len);
			content += escape_cks_len;
			content[0] = FRAME_MARK;
			content += 1;
			ret = int(content - buffer);
		}
	}
	return ret;
}

int Parse(const byte* frame, int frame_length, byte* buffer, int buffer_size, int * extData)
{
	int InvertEscapeMessage(const byte * emsg, int emsg_length, byte * buffer, int buffer_size, int *extData);
	int errorCode = EC_SUCCEED;
	if (extData != nullptr)
	{
		*extData = 0;
	}
	if (frame_length > 0 && frame != nullptr)
	{
		if (frame_length < MIN_FRAME_LEN)
		{
			errorCode = EC_ILLEGAL_FRAME;
			if (extData != nullptr)
			{
				*extData = ECIF_SHORT_THEN_MIN_FRAME_LENGTH;
			}
		}

		if (frame[0] != FRAME_MARK)
		{
			errorCode = EC_ILLEGAL_FRAME;
			if (extData != nullptr)
			{
				*extData |= ECIF_HEAD;
			}
		}

		if (frame[frame_length - 1] != FRAME_MARK)
		{
			errorCode = EC_ILLEGAL_FRAME;
			if (extData != nullptr)
			{
				*extData |= ECIF_TAIL;
			}
		}
		
		if (errorCode == EC_SUCCEED)
		{
			int data_size = 0;
			if (extData != nullptr)
			{
				*extData = 0;
			}

			const byte* esc_msg_cks = frame + 1;
			const int emsg_cks_len = frame_length - 2;
			errorCode = InvertEscapeMessage(esc_msg_cks, emsg_cks_len, buffer, buffer_size, extData);
			data_size = *extData;
			if (errorCode == EC_SUCCEED && data_size > 0)
			{
				byte cks = cks_current(buffer, data_size - 1);
				byte cks_msg = buffer[data_size - 1];
				if (cks == cks_msg)
				{
					errorCode = EC_SUCCEED;
					if (extData != nullptr)
					{
						*extData = data_size - 1;
					}
				}
				else
				{
					errorCode = EC_INCORRECT_CKS;
					if (extData != nullptr)
					{
						*extData = ((cks << 8) & 0xff00) | (cks_msg & 0xff);
					}
				}
			}
		}
	}
	
	return errorCode;;
}

int ReadFrames(const byte* buffer, int buffer_size, IFrameProc proc)
{
	int tail_size = buffer_size;
	if (buffer_size > 0 && buffer != nullptr)
	{
#define STATE_NONE 0
#define STATE_START 1
#define STATE_END 2


#define TRANSITION(from, condition, to, code)										\
if ((state == (from)) && (condition)) { state = (to); code; continue;  }			\

		byte state = STATE_NONE;
		int frame_begin = 0;
		const byte FrameToken = FRAME_MARK;
		for (int i = 0; i < buffer_size; ++i)
		{
			bool isFrameTag = buffer[i] == FrameToken;
			TRANSITION(STATE_NONE, isFrameTag, STATE_START, { frame_begin = i; });
			TRANSITION(STATE_NONE, !isFrameTag, STATE_NONE, {});
			TRANSITION(STATE_START, !isFrameTag, STATE_START, { });
			TRANSITION(STATE_START, isFrameTag && (i - frame_begin + 1 >= MIN_FRAME_LEN), STATE_END, {
				if(proc != nullptr)
				{
					proc(buffer + frame_begin, i - frame_begin + 1);
				}
			});
			TRANSITION(STATE_START, isFrameTag && (i - frame_begin + 1 < MIN_FRAME_LEN), STATE_START, {	frame_begin = i;});
			TRANSITION(STATE_END, !isFrameTag, STATE_NONE, {});
			TRANSITION(STATE_END, isFrameTag, STATE_START, { frame_begin = i; });
		}
		switch (state)
		{
		case STATE_START:
			tail_size = buffer_size - frame_begin;
			break;
		case STATE_NONE:
		case STATE_END:
		default:
			tail_size = 0;
			break;
		}
#undef TRANSITION
#undef STATE_NONE
#undef STATE_START
#undef STATE_END
	}
	return tail_size;

}

int SetMessageBaseSize(int bytes)
{
	int old = min_message_length;
	min_message_length = bytes;
	return old;
}


/// <summary>
/// 计算原消息转义后的大小
/// </summary>
/// <param name="message"></param>
/// <param name="msg_length"></param>
/// <returns></returns>
int GetEscapeMessageSize(const byte* message, int msg_length)
{
	int size = 0;
	if (message == nullptr || msg_length == 0)
	{
		return size;
	}
	for (int i = 0; i < msg_length; ++i)
	{
		if (message[i] == FRAME_MARK || message[i] == FRAME_ESCAPE)
		{
			size += 2;
		}
		else
		{
			size += 1;
		}
	}
	return size;
}

#define OPTIMIZED_MEMORY_COPY


#ifdef OPTIMIZED_MEMORY_COPY

#if defined _MEMCPY
#error _MEMCPY id alread defined
#endif

#if _MSVC_
#define _MEMCPY(d, len_d, s, len_s) memcpy_s(d, len_d, s, _lens)
#else
#define _MEMCPY(d, len_d, s, len_s) memcpy(d, s, len_s)
#endif


#endif

/// <summary>
/// 将原消息进行转义，并将数据写入"网络数据缓存" 中
/// 当buffer 为 nullptr时，返回构建转义序列所需的缓存长度
/// 返回值小于0表示缓存区buffer 长度不够, 其绝对值为期望长度
/// </summary>
/// <param name="message">原始消息</param>
/// <param name="msg_length">原始消息长度</param>
/// <param name="buffer">转义消息缓存</param>
/// <param name="buffer_size">转义消息缓存长度</param>
/// <returns>转义消息在buffer中的实际长度</returns>
int EscapeMessage(const byte* message, int msg_length, byte* buffer, int buffer_size)
{
	int escape_msg_length = GetEscapeMessageSize(message, msg_length);
	if (buffer != nullptr)
	{
		if (escape_msg_length <= buffer_size)
		{
			byte* content = buffer;
			int content_size = buffer_size;
			int startIndex = 0;
			int endIndex = 0;
			int copy_size = 0;
			for (; endIndex < msg_length; ++endIndex)
			{
#if defined OPTIMIZED_MEMORY_COPY
				if (message[endIndex] == FRAME_MARK)
				{
					copy_size = endIndex - startIndex;
                    _MEMCPY(content, content_size, message + startIndex, copy_size);
					content_size -= copy_size;
					content += copy_size;
					content[0] = FRAME_ESCAPE;
					content[1] = FRAME_ESC_MARK;
					content += 2;
					startIndex = endIndex + 1;
				}
				else if (message[endIndex] == FRAME_ESCAPE)
				{
					copy_size = endIndex - startIndex;
                    _MEMCPY(content, content_size, message + startIndex, copy_size);
					content_size -= copy_size;
					content += copy_size;
					content[0] = FRAME_ESCAPE;
					content[1] = FRAME_ESC_ESCAPE;
					content += 2;
					startIndex = endIndex + 1;
				}
				else
				{
				}
#else
				if (message[endIndex] == FRAME_MARK)
				{
					content[0] = FRAME_ESCAPE;
					content[1] = FRAME_ESC_MARK;
					content += 2;
				}
				else if (message[endIndex] == FRAME_ESCAPE)
				{
					content[0] = FRAME_ESCAPE;
					content[1] = FRAME_ESC_ESCAPE;
					content += 2;
				}
				else
				{
					content[0] = message[endIndex];
					content += 1;
				}
#endif
			}
#if defined OPTIMIZED_MEMORY_COPY
			copy_size = endIndex - startIndex;
			if (copy_size > 0)
			{
                _MEMCPY(content, content_size, message + startIndex, copy_size);
				content_size -= copy_size;
			}
#endif
		}
		else
		{
			escape_msg_length = -escape_msg_length;
		}
	}

	return escape_msg_length;
}

int GetOriginalMessageSize(const byte* esc_msg, int esc_msg_len)
{
	int size = 0;
	if (esc_msg != nullptr && esc_msg_len != 0)
	{
		for (int i = 0; i < esc_msg_len; ++i)
		{
			if (esc_msg[i] == FRAME_MARK)
			{
				goto ILLEGAL_EMSG;
			}
			if (esc_msg[i] == FRAME_ESCAPE)
			{
				int esc_id_index = i + 1;
				if (esc_id_index < esc_msg_len
					&& (esc_msg[esc_id_index] == FRAME_ESC_MARK || esc_msg[esc_id_index] == FRAME_ESC_ESCAPE)
					)
				{
					size += 1;
					i += 1;
				}
				else
				{
					goto ILLEGAL_EMSG;
				}
			}
			else
			{
				size += 1;
			}
		}
		goto EXIT;
	ILLEGAL_EMSG:
		size = -1;
	}
EXIT:
	return size;
}

int InvertEscapeMessage(const byte* emsg, int emsg_length, byte* buffer, int buffer_size, int* extData)
{
	int errorCode = EC_SUCCEED;
	if (extData != nullptr)
	{
		*extData = 0;
	}
	if (emsg != nullptr && emsg_length > 0)
	{

		int msg_len = GetOriginalMessageSize(emsg, emsg_length);
		if (msg_len <= 0) // 包含非法的转义
		{
			errorCode = EC_ILLEGAL_FRAME;
			if (extData != nullptr)
			{
				*extData = ECIF_ILLEGAL_ESCAPE;
			}
		}
		else
		{
			if (buffer_size < msg_len || buffer == nullptr)
			{
				errorCode = EC_NOT_ENOUGH_MEMORY;
				if (extData != nullptr)
				{
					*extData = msg_len;
				}
			}
			else
			{
				//errorCode = EC_SUCCEED;
				//if (extData != nullptr)
				//{
				//	*extData = 0;
				//}
				int beginIndex = 0, endIndex = 0;
				int buffer_write_index = 0;
				int content_size = 0;
#ifndef OPTIMIZED_MEMORY_COPY
				for (int endIndex = 0; endIndex < emsg_length; ++endIndex, ++buffer_write_index)
				{
					if (emsg[endIndex] == FRAME_ESCAPE)
					{
						int esc_id_index = endIndex + 1;
						if (esc_id_index < emsg_length)
						{
							byte esc_id = emsg[esc_id_index];
							if (esc_id == FRAME_ESC_MARK)
							{
								buffer[buffer_write_index] = FRAME_MARK;
							}
							else if (esc_id == FRAME_ESC_ESCAPE)
							{
								buffer[buffer_write_index] = FRAME_ESCAPE;
							}
							else
							{
								errorCode = EC_ILLEGAL_FRAME;
								if (extData != nullptr)
								{
									*extData = ECIF_ILLEGAL_ESCAPE;
								}
								break;
							}
							++endIndex;
						}
					}
					else
					{
						buffer[buffer_write_index] = emsg[endIndex];
					}
				}
				if (errorCode == EC_SUCCEED)
				{
					if (extData != nullptr)
					{
						*extData = buffer_write_index;
					}
				}
#else
				for (; endIndex < emsg_length; ++endIndex)
				{
					if (emsg[endIndex] == FRAME_ESCAPE)
					{
						content_size = endIndex - beginIndex;
                        _MEMCPY(buffer + buffer_write_index, content_size, emsg + beginIndex, content_size);
						buffer_write_index += content_size;
						int esc_id_index = endIndex + 1;
						if (esc_id_index < emsg_length)
						{
							byte esc_id = emsg[esc_id_index];
							if (esc_id == FRAME_ESC_MARK)
							{
								buffer[buffer_write_index] = FRAME_MARK;
								buffer_write_index += 1;
							}
							else if (esc_id == FRAME_ESC_ESCAPE)
							{
								buffer[buffer_write_index] = FRAME_ESCAPE;
								buffer_write_index += 1;
							}
							else
							{
								errorCode = EC_ILLEGAL_FRAME;
								if (extData != nullptr)
								{
									*extData = ECIF_ILLEGAL_ESCAPE;
								}
								break;
							}
						}
						endIndex = esc_id_index;
						beginIndex = endIndex + 1;
					}
				}
#endif
				if (errorCode == EC_SUCCEED)
				{
#if defined OPTIMIZED_MEMORY_COPY
					content_size = endIndex - beginIndex;
					if (content_size > 0)
					{
                        _MEMCPY(buffer + buffer_write_index, content_size, emsg + beginIndex, content_size);
					}
#endif
					if (extData != nullptr)
					{
						*extData = buffer_write_index + content_size;
					}
				}
			}
		}
	}
	return errorCode;
}
