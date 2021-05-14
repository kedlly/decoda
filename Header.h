#pragma once
typedef unsigned char byte;
int find_first_byte_0(byte* dataPtr, int offset, int count, byte value)
{

	if (count == 0 || dataPtr == nullptr)
	{
		// 没找到
		return -1;
	}
	// 4字节对齐
	byte* pByte = dataPtr + offset;
	while (count > 0)
	{
		if (*pByte == value)
		{
			return (int)(pByte - dataPtr);
		}
		--count;
		++pByte;
	}
	return -1;
}


int find_first_byte_32(byte* dataPtr, int offset, int count, byte value)
{
	if (count == 0 || dataPtr == nullptr)
	{
		// 没找到
		return -1;
	}
	// 4字节对齐
	byte* pByte = dataPtr + offset;
	while (((int)pByte & 3) != 0)
	{
		if (count == 0)
		{
			return -1;
		}
		else if (*pByte == value)
		{
			return (int)(pByte - dataPtr);
		}
		else {}
		--count;
		++pByte;
	}

	// Fill comparer with value byte for comparisons
	//
	// comparer = value/value/value/value
	unsigned int comparer = (unsigned int)value | ((unsigned int)value << 8);
	comparer |= comparer << 16; //32bit

	while (count > 3)
	{
		unsigned t1 = *(unsigned int*)pByte;
		t1 ^= comparer;
		unsigned t2 = 0x7efefeff + t1;
		t1 ^= 0xffffffff;
		t1 ^= t2;
		t1 &= 0x81010100;
		if (t1 != 0)
		{
			int foundIndex = (int)(pByte - dataPtr);
			if (pByte[0] == value)
				return foundIndex;
			else if (pByte[1] == value)
				return foundIndex + 1;
			else if (pByte[2] == value)
				return foundIndex + 2;
			else if (pByte[3] == value)
				return foundIndex + 3;
			else {}
		}
		count -= 4;
		pByte += 4;
	}

	// Catch any bytes that might be left at the tail of the buffer
	while (count > 0)
	{
		if (*pByte == value)
			return (int)(pByte - dataPtr);

		--count;
		++pByte;
	}

	// If we don't have a match return -1;
	return -1;
}


int find_first_byte_64(byte* dataPtr, int offset, int count, byte value)
{
	if (count == 0 || dataPtr == nullptr)
	{
		// 没找到
		return -1;
	}
	// 4字节对齐
	byte* pByte = dataPtr + offset;
	while (((int)pByte & (8 - 1)) != 0)
	{
		if (count == 0)
		{
			return -1;
		}
		else if (*pByte == value)
		{
			return (int)(pByte - dataPtr);
		}
		else {}
		--count;
		++pByte;
	}

	// Fill comparer with value byte for comparisons
	//
	// comparer = value/value/value/value
	unsigned long long comparer = (unsigned long long)value | ((unsigned long long)value << 8);
	comparer |= comparer << 16; //32bit
	comparer |= comparer << 32; //64bit

	while (count > (8 - 1))
	{
		unsigned long long t1 = *(unsigned long long*)pByte;
		t1 ^= comparer;
		unsigned long long t2 = 0x7efefefefefefeff + t1;
		t1 ^= 0xffffffffffffffff;
		t1 ^= t2;
		t1 &= 0x8101010101010100;
		if (t1 != 0)
		{/*
			int foundIndex = (int)(pByte - dataPtr);
			for (int i = 0; i < 8; ++i)
			{
				if (pByte[i] == value)
				{
					return foundIndex + i;
				}
			}*/
			int foundIndex = (int)(pByte - dataPtr);
			if (pByte[0] == value)
				return foundIndex;
			else if (pByte[1] == value)
				return foundIndex + 1;
			else if (pByte[2] == value)
				return foundIndex + 2;
			else if (pByte[3] == value)
				return foundIndex + 3;
			else if (pByte[4] == value)
				return foundIndex + 4;
			else if (pByte[5] == value)
				return foundIndex + 5;
			else if (pByte[6] == value)
				return foundIndex + 6;
			else if (pByte[7] == value)
				return foundIndex + 7;
			else {}
		}
		count -= 8;
		pByte += 8;
	}

	// Catch any bytes that might be left at the tail of the buffer
	while (count > 0)
	{
		if (*pByte == value)
			return (int)(pByte - dataPtr);

		--count;
		++pByte;
	}

	// If we don't have a match return -1;
	return -1;
}