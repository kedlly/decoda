#ifndef __I_NET_FRAME__
#define __I_NET_FRAME__


#define DEFAULT_FRAME_MARK					0x7e
#define DEFAULT_FRAME_ESCAPE				0x7d
#define DEFAULT_FRAME_ESC_MARK				0x02
#define DEFAULT_FRAME_ESC_ESCAPE			0x01


typedef unsigned char byte;

struct FrameDefinition
{
	byte Mark;						//��ʶλ
	byte Escape;					//ת��ǰ��
	byte Esc_Mark;					//��ʶλ��Ϣֵ
	byte Esc_Escape;				//ת��ǰ����Ϣֵ
};

/*
* ����֡��ʽ
* |-----|--------------|-------------|------|
* |��ʶλ|��Ϣ��(ת������)|У����(ת������)|��ʶλ |
* |-----|--------------|-------------|------|
* 
* ��ʶλ һ���ֽ� INET_FRAME_ID {FrameDefinition.Mark}
* 
* ��У���롢��Ϣ���г��� INET_FRAME_ID����Ҫ����ת�崦��ת��
* ���������£�
* 1 �� FrameDefinition �ṹ�ж���
* 2 Mark     <--> Escape �����һ�� Esc_Mark
* 3 Escape   <--> Escape �����һ�� Esc_Escape
* �Ӷ�ʹ����Ϣ�����в�����"��ʶλ"��
* 
* У���� ռ��һ���ֽ�; ͨ���ض��㷨�������Ϣ��ʼ��У����ǰһ���ֽڵ�У��ֵ
* 
* ������Ϣʱ����Ϣ��װ����>���㲢���У���롪��>ת�壻
* ������Ϣʱ��ת�廹ԭ����>��֤У���롪��>������Ϣ
* 
*/

/// <summary>
/// У��ͼ��㷽��
/// </summary>
typedef byte (*CalcCKS)(const byte* stream, int stream_length);

/// <summary>
/// ������������֡����������д��"�������ݻ���" ��
/// ��buffer Ϊ nullptrʱ�����ع�������֡����Ļ��泤��
/// ����ֵС��0��ʾ������buffer ���Ȳ��� ,�����ֵΪ�貹�䳤��
/// </summary>
/// <param name="message">ԭʼ��Ϣ</param>
/// <param name="msg_length">ԭʼ��Ϣ����</param>
/// <param name="buffer">��������֡����</param>
/// <param name="buffer_size">��������֡���泤��</param>
/// <returns>��������֡��buffer�е�ʵ�ʳ���</returns>
int Build(const byte* message , int msg_length, byte* buffer, int buffer_size);

#define EC_SUCCEED                          0      //�ɹ�
#define EC_NOT_ENOUGH_MEMORY                1      //���治��
#define EC_INCORRECT_CKS                    2      //У��ʹ���
#define EC_ILLEGAL_FRAME					3      //֡��Ϣ����

#define ECIF_OK								0
#define ECIF_HEAD                           (1 << 0)      //��ʾ֡ͷ���ݴ���
#define ECIF_TAIL                           (1 << 1)      //��ʾ֡β���ݴ���
#define ECIF_ILLEGAL_ESCAPE                 (1 << 2)      //��ʾ֡�����д��ڷǷ���ת������
#define ECIF_SHORT_THEN_MIN_FRAME_LENGTH    (1 << 3)      //��ʾ֡���ݳ��Ȳ�������С֡��������

/// <summary>
/// ��"��������֡����"����ȡ"��Ϣ"��������Ϣд����Ϣ����
/// ����ֵ Ϊ EC_SUCCEED ʱ            extData����buffer����Ϣ��ʵ���ֽ���
///       Ϊ EC_NOT_ENOUGH_MEMORY ʱ ��ʾ������buffer ���Ȳ���, extDataֵΪ������Ϣ�������С����
///       Ϊ EC_INCORRECT_CKS ʱ��ʾ ��Ϣ����֤У��Ͳ����˴��� extDataֵΪ ��һ�ֽ�Ϊʵ��У��ͣ��ڶ��ֽ�Ϊ��ǰcks���㺯���ó���У���
///		  Ϊ EC_ILLEGAL_FRAME ʱ��ʾ֡��Ϣ����
///                    extDataֵ �� ECIF_* �������
/// </summary>
/// <param name="frame">��������֡</param>
/// <param name="frame_length">��������֡����</param>
/// <param name="buffer">��Ϣ����</param>
/// <param name="buffer_size">��Ϣ���泤��</param>
/// <param name="extData">��������</param>
/// <returns>������</returns>
int Parse(const byte* frame, int frame_length, byte* buffer, int buffer_size, int* extData);

/// <summary>
/// ���� ֡��ʶ��֡ת���ֽڶ���
/// ����ԭ����
/// Ĭ�϶���Ϊ {INET_FRAME_ID, INET_FRAME_ESCAPE, INET_FRAME_ESC_ID, INET_FRAME_ESC_ESCAPE}
/// </summary>
/// <param name="ie">Ŀ�궨��</param>
/// <returns>ԭ����</returns>
FrameDefinition SetIdentifierEscape(FrameDefinition ie);

/// <summary>
/// ���� ֡����У����㺯��
/// ����ԭ����
/// У����㺯��ָ��ΪNULLʱУ��ֵǿ��Ϊ0��Ĭ�ϼ��㺯��Ϊ������Ϣ�������ֽ�xorֵ
/// </summary>
/// <param name="target">Ŀ�꺯��</param>
/// <returns>ԭ����</returns>
CalcCKS SetCKS(CalcCKS target);


typedef void (*IFrameProc)(const byte* frame, const int frame_len);


/// <summary>
/// �ӻ����а�˳��ȡ������֡�������ô������̽��д���
/// </summary>
/// <param name="buffer">���ݻ���</param>
/// <param name="buffer_size">���ݻ��泤��</param>
/// <param name="proc">����֡��������</param>
/// <returns>buffer�а���������֡����proc������Ϻ�ʣ������ݳ���</returns>
int ReadFrames(const byte* buffer, int buffer_size, IFrameProc proc);

/// <summary>
/// ������Ϣ����С�ߴ�(��λΪ�ֽ���)
/// </summary>
/// <param name="bytes">�ֽ���</param>
/// <returns>ԭ�ߴ���ֵ</returns>
int SetMessageBaseSize(int bytes);

#endif //__I_NET_FRAME__