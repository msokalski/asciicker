
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"

#ifdef _WIN32

#pragma comment(lib,"Ws2_32.lib")

int TCP_INIT()
{
	WSADATA wsaData;
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int TCP_CLOSE(TCP_SOCKET s)
{
	return closesocket(s);
}

int TCP_CLEANUP()
{
	return WSACleanup();
}

struct THREAD_HANDLE
{
	HANDLE th;

	void* (*entry)(void*);
	void* arg;

	static DWORD WINAPI wrap(LPVOID p)
	{
		THREAD_HANDLE* t = (THREAD_HANDLE*)p;
		t->arg = t->entry(t->arg);
		return 0;
	}

	static DWORD WINAPI wrap_detached(LPVOID p)
	{
		THREAD_HANDLE* t = (THREAD_HANDLE*)p;
		void* arg = t->arg;
		void*(*entry)(void*) = t->entry;
		free(t);
		entry(arg);
		return 0;
	}
};

THREAD_HANDLE* THREAD_CREATE(void* (*entry)(void*), void* arg)
{
	HANDLE th;
	DWORD id;

	THREAD_HANDLE* t = (THREAD_HANDLE*)malloc(sizeof(THREAD_HANDLE));
	t->arg = arg;
	t->entry = entry;
	th = CreateThread(0, 0, THREAD_HANDLE::wrap, t, 0, &id);
	if (!th)
	{
		free(t);
		return 0;
	}
	t->th = th;
	return t;
}

void* THREAD_JOIN(THREAD_HANDLE* thread)
{
	WaitForSingleObject(thread->th, INFINITE);
	CloseHandle(thread->th);
	void* ret = thread->arg;
	free(thread);
	return ret;
}


bool THREAD_CREATE_DETACHED(void* (*entry)(void*), void* arg)
{
	DWORD id;
	THREAD_HANDLE* t = (THREAD_HANDLE*)malloc(sizeof(THREAD_HANDLE));
	t->arg = arg;
	t->entry = entry;
	HANDLE th = CreateThread(0, 0, THREAD_HANDLE::wrap_detached, t, 0, &id);
	if (!th)
	{
		free(t);
		return false;
	}
	CloseHandle(th);
	return true;
}

void THREAD_SLEEP(int ms)
{
	Sleep(ms);
}

struct RWLOCK_HANDLE
{
	SRWLOCK rw;
};

RWLOCK_HANDLE* RWLOCK_CREATE()
{
	RWLOCK_HANDLE* rwl = (RWLOCK_HANDLE*)malloc(sizeof(RWLOCK_HANDLE));
	InitializeSRWLock(&rwl->rw);
	return rwl;
}

void RWLOCK_DELETE(RWLOCK_HANDLE* rwl)
{
	free(rwl);
}

void RWLOCK_READ_LOCK(RWLOCK_HANDLE* rwl)
{
	AcquireSRWLockShared(&rwl->rw);
}

void RWLOCK_READ_UNLOCK(RWLOCK_HANDLE* rwl)
{
	ReleaseSRWLockShared(&rwl->rw);
}

void RWLOCK_WRITE_LOCK(RWLOCK_HANDLE* rwl)
{
	AcquireSRWLockExclusive(&rwl->rw);
}

void RWLOCK_WRITE_UNLOCK(RWLOCK_HANDLE* rwl)
{
	ReleaseSRWLockExclusive(&rwl->rw);
}

unsigned int INTERLOCKED_DEC(volatile unsigned int* ptr)
{
	return InterlockedDecrement(ptr);
}

unsigned int INTERLOCKED_INC(volatile unsigned int* ptr)
{
	return InterlockedIncrement(ptr);
}

unsigned int INTERLOCKED_SUB(volatile unsigned int* ptr, unsigned int sub)
{
	return (unsigned int)InterlockedAdd((volatile LONG*)ptr,-(LONG)sub);
}

unsigned int INTERLOCKED_ADD(volatile unsigned int* ptr, unsigned int add)
{
	return (unsigned int)InterlockedAdd((volatile LONG*)ptr, (LONG)add);
}


struct MUTEX_HANDLE
{
	CRITICAL_SECTION mu;
};

MUTEX_HANDLE* MUTEX_CREATE()
{
	MUTEX_HANDLE* m = (MUTEX_HANDLE*)malloc(sizeof(MUTEX_HANDLE));
	InitializeCriticalSection(&m->mu);
	return m;
}

void MUTEX_DELETE(MUTEX_HANDLE* mutex)
{
	DeleteCriticalSection(&mutex->mu);
	free(mutex);
}

void MUTEX_LOCK(MUTEX_HANDLE* mutex)
{
	EnterCriticalSection(&mutex->mu);
}

void MUTEX_UNLOCK(MUTEX_HANDLE* mutex)
{
	LeaveCriticalSection(&mutex->mu);
}

#else

typedef int TCP_SOCKET;
#define INVALID_TCP_SOCKET (-1)

int TCP_INIT()
{
	return 0;
}

int TCP_CLOSE(TCP_SOCKET s)
{
	return close(s);
}

int TCP_CLEANUP()
{
	return 0;
}

struct THREAD_HANDLE
{
	pthread_t th;
};

THREAD_HANDLE* THREAD_CREATE(void* (*entry)(void*), void* arg)
{
	pthread_t th;
	pthread_create(&th, 0, entry, arg);
	if (!th)
		return 0;

	THREAD_HANDLE* t = (THREAD_HANDLE*)malloc(sizeof(THREAD_HANDLE));
	t->th = th;
	return t;
}

void* THREAD_JOIN(THREAD_HANDLE* thread)
{
	void* ret = 0;
	pthread_join(thread->th, &ret);
	free(thread);
	return ret;
}

bool THREAD_CREATE_DETACHED(void* (*entry)(void*), void* arg)
{
	pthread_t th;
	pthread_create(&th, 0, entry, arg);
	if (!th)
		return false;
	pthread_detach(th);
	return true;
}

void THREAD_SLEEP(int ms)
{
	usleep(ms*1000);
}

struct RWLOCK_HANDLE
{
	pthread_rwlock_t rw;
};

RWLOCK_HANDLE* RWLOCK_CREATE()
{
	RWLOCK_HANDLE* rwl = (RWLOCK_HANDLE*)malloc(sizeof(RWLOCK_HANDLE));
	pthread_rwlock_init(&rwl->rw, 0);
	return rwl;
}

void RWLOCK_DELETE(RWLOCK_HANDLE* rwl)
{
	pthread_rwlock_destroy(&rwl->rw);
	free(rwl);
}

void RWLOCK_READ_LOCK(RWLOCK_HANDLE* rwl)
{
	pthread_rwlock_rdlock(&rwl->rw);
}

void RWLOCK_READ_UNLOCK(RWLOCK_HANDLE* rwl)
{
	pthread_rwlock_unlock(&rwl->rw);
}

void RWLOCK_WRITE_LOCK(RWLOCK_HANDLE* rwl)
{
	pthread_rwlock_wrlock(&rwl->rw);
}

void RWLOCK_WRITE_UNLOCK(RWLOCK_HANDLE* rwl)
{
	pthread_rwlock_unlock(&rwl->rw);
}

struct MUTEX_HANDLE
{
	pthread_mutex_t mu;
};

MUTEX_HANDLE* MUTEX_CREATE()
{
	MUTEX_HANDLE* m = (MUTEX_HANDLE*)malloc(sizeof(MUTEX_HANDLE));
	pthread_mutex_init(&m->mu, 0);
	return m;
}

void MUTEX_DELETE(MUTEX_HANDLE* mutex)
{
	pthread_mutex_destroy(&mutex->mu);
	free(mutex);
}

void MUTEX_LOCK(MUTEX_HANDLE* mutex)
{
	pthread_mutex_lock(&mutex->mu);
}

void MUTEX_UNLOCK(MUTEX_HANDLE* mutex)
{
	pthread_mutex_unlock(&mutex->mu);
}

unsigned int INTERLOCKED_DEC(volatile unsigned int* ptr)
{
	return __sync_fetch_and_sub(ptr, 1) - 1;
}

unsigned int INTERLOCKED_INC(volatile unsigned int* ptr)
{
	return __sync_fetch_and_add(ptr, 1) + 1;
}

unsigned int INTERLOCKED_SUB(volatile unsigned int* ptr, unsigned int sub)
{
	return __sync_fetch_and_sub(ptr, sub) - sub;
}

unsigned int INTERLOCKED_ADD(volatile unsigned int* ptr, unsigned int add)
{
	return __sync_fetch_and_add(ptr, add) + add;
}

#endif

int TCP_WRITE(TCP_SOCKET s, const uint8_t* buf, int size)
{
	int l = size;
	while (l > 0)
	{
		int w = send(s, (const char*)buf, l, 0);
		if (w <= 0)
			return w;
		l -= w;
		buf += w;		
	}
	return size;
}

int TCP_READ(TCP_SOCKET s, uint8_t* buf, int size)
{
	int l = size;
	while (l > 0)
	{
		int r = recv(s, (char*)buf, l, 0);
		if (r <= 0)
			return r;
		l -= r;
		buf += r;
	}
	return size;
}

// returns body_overread size (so 0 is very fine)
int HTTP_READ(TCP_SOCKET s, int(*cb)(const char* header, const char* value, void* param), void* param, char body_overread[2048])
{
	int crlf_state = 0;
	int header_len = -1;
	int value_len = 0;
	int col = 1; // currently header or value

	uint8_t buf[2048];

	int ret = 0;

	int header_size = 1024;
	int value_size = 1024;
	char* header = (char*)malloc(1024 + 1);
	char* value = (char*)malloc(1024 + 1);

	do
	{
		int r = recv(s, (char*)buf, 2048, 0);
		if (r <= 0)
		{
			free(header);
			free(value);
			if (body_overread)
				body_overread[0] = 0;
			return r;
		}

		for (int i = 0; i < r; i++)
		{
			switch (buf[i])
			{
			case 0x0D:
			{
				if (crlf_state == 0 || crlf_state == 2)
					crlf_state++;
				else
					crlf_state = 0;
				break;
			}

			case 0x0A:
			{
				if (crlf_state == 1 || crlf_state == 3)
				{
					if (col == 1)
					{
						value[value_len] = 0;
						if (header_len < 0)
							ret = cb(0, value, param);
						else
						{
							header[header_len] = 0;
							ret = cb(header, value, param);
						}

						if (ret < 0)
						{
							free(header);
							free(value);
							if (body_overread)
								body_overread[0] = 0;
							return ret;
						}
					}

					value_len = -1;
					header_len = 0;
					crlf_state++;
					col = 0;
				}
				else
					crlf_state = 0;
				break;
			}

			default:
			{
				crlf_state = 0;
				if (col == 0)
				{
					if (buf[i] == ':')
					{
						col = 1;
						value_len = -1;
					}
					else
					{
						if (header_len == header_size)
						{
							if (header_size >= 65536)
							{
								free(header);
								free(value);
								if (body_overread)
									body_overread[0] = 0;
								return -2;
							}
							header_size *= 2;
							header = (char*)realloc(header, header_size + 1);
						}
						header[header_len] = buf[i];
						header_len++;
					}
				}
				else
				{
					if (value_len == -1)
					{
						if (buf[i] != ' ')
						{
							free(header);
							free(value);
							if (body_overread)
								body_overread[0] = 0;
							return -2;
						}
						value_len++;
					}
					else
					{
						if (value_len == value_size)
						{
							if (value_size >= 65536)
							{
								free(header);
								free(value);
								if (body_overread)
									body_overread[0] = 0;
								return -2;
							}

							value_size *= 2;
							value = (char*)realloc(value, value_size + 1);
						}
						value[value_len] = buf[i];
						value_len++;
					}
				}
				break;
			}
			}

			if (crlf_state == 4)
			{
				i++;
				ret = r - i;
				if (body_overread)
					memcpy(body_overread, buf + i, ret);
				break;
			}
		}

	} while (crlf_state != 4);

	free(header);
	free(value);

	return ret;
}

int WS_WRITE(TCP_SOCKET s, const uint8_t* buf, int size, int split, int type)
{
	if (size <= 0)
		return size;

	if (split <= 0)
		split = size;

	// try making frames of equal size
	int frames = (size + split - 1) / split;
	int frame_size = (size + frames - 1) / frames;

	/*
		  0                   1                   2                   3
		  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 +-+-+-+-+-------+-+-------------+-------------------------------+
		 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
		 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
		 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
		 | |1|2|3|       |K|             |                               |
		 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
		 |     Extended payload length continued, if payload len == 127  |
		 + - - - - - - - - - - - - - - - +-------------------------------+
		 |                               |Masking-key, if MASK set to 1  |
		 +-------------------------------+-------------------------------+
		 | Masking-key (continued)       |          Payload Data         |
		 +-------------------------------- - - - - - - - - - - - - - - - +
		 :                     Payload Data continued ...                :
		 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
		 |                     Payload Data continued ...                |
		 +---------------------------------------------------------------+
	*/

	int offs = 0;

	int fmt = type;

	do
	{
		int len = 0;
		uint8_t frame[10];

		int payload = frame_size;

		if (offs + payload >= size)
		{
			payload = size - offs;
			frame[0] = 0x80/*FIN*/;
		}
		else
		{
			frame[0] = 0x00/*FIN*/;
		}

		if (offs == 0)
			frame[0] |= fmt/*BIN*/;

		if (payload < 126)
		{

			frame[1] = 0x00/*MSK*/ | payload;
			len = 2;
		}
		else
			if (payload < 65536)
			{
				frame[1] = 0x00/*MSK*/ | 126;
				frame[2] = (payload >> 8) & 0xFF;
				frame[3] = payload & 0xFF;
				len = 4;
			}
			else
			{
				frame[1] = 0x00/*MSK*/ | 127;
				frame[2] = 0;
				frame[3] = 0;
				frame[4] = 0;
				frame[5] = 0;
				frame[6] = (payload >> 24) & 0xFF;
				frame[7] = (payload >> 16) & 0xFF;
				frame[8] = (payload >> 8) & 0xFF;
				frame[9] = payload & 0xFF;
				len = 10;
			}

		int w = TCP_WRITE(s, frame, len);
		if (w <= 0)
			return w;

		if (payload)
		{
			int w = TCP_WRITE(s, buf + offs, payload);
			if (w <= 0)
				return w;
			offs += w;
		}
	} while (offs < size);

	return size;
}

int WS_READ(TCP_SOCKET s, uint8_t* buf, int size, int* type)
{
	if (size < 0)
		return size;

	int len = 0, tot_data = 0;
	uint8_t frame[14];

	do
	{
		// read first 2 bytes
		{
			int r = TCP_READ(s, frame, 2);
			if (r <= 0)
				return r;
			len += r;
		}

		if (type)
			*type = frame[0] & 0xF;

		uint64_t payload = frame[1] & 0x7F;
		uint8_t* mask = 0;

		if (frame[1] & 0x80) // if mask bit is set
		{

			if (payload == 126)
			{
				// READ next 6 bytes -> first 2 replace payload len, other 4 is xor_mask
				int r = TCP_READ(s, frame + len, 6);
				if (r <= 0)
					return r;
				len += r;

				payload = (frame[2] << 8) | frame[3];
				mask = frame + 4;
			}
			else
				if (payload == 127)
				{
					// READ next 12 bytes -> first 8 replace payload len, other 4 is xor_mask
					int r = TCP_READ(s, frame + len, 12);
					if (r <= 0)
						return r;
					len += r;

					payload = (frame[2] << 24) | (frame[3] << 16) | (frame[4] << 8) | frame[5];
					payload |= (payload << 32) | (frame[6] << 24) | (frame[7] << 16) | (frame[8] << 8) | frame[9];

					mask = frame + 10;
				}
				else
				{
					// READ next 4 bytes of xor_mask
					int r = TCP_READ(s, frame + len, 4);
					if (r <= 0)
						return r;
					len += r;

					mask = frame + 2;
				}
		}
		else
		{
			if (payload == 126)
			{
				// READ next 2 bytes->replace payload len
				int r = TCP_READ(s, frame + len, 2);
				if (r <= 0)
					return r;
				len += r;

				payload = (frame[2] << 8) | frame[3];
			}
			else
				if (payload == 127)
				{
					// READ next 8 bytes->replace payload len
					int r = TCP_READ(s, frame + len, 8);
					if (r <= 0)
						return r;
					len += r;

					payload = (frame[2] << 24) | (frame[3] << 16) | (frame[4] << 8) | frame[5];
					payload |= (payload << 32) | (frame[6] << 24) | (frame[7] << 16) | (frame[8] << 8) | frame[9];
				}
		}

		if (size < payload)
		{
			return -1;
		}

		switch (frame[0] & 0xF)
		{
			case 0x0:
			case 0x1:
			case 0x2:
				break;

			case 0x8: // close
			{
				WS_WRITE(s, 0, 0, 0, 0x8);
				return -1;
			}
			case 0x9: // ping
			{
				uint8_t ping[125];
				if (payload)
				{
					int r = TCP_READ(s, ping, payload);
					if (r <= 0)
						return r;
					if (mask)
					{
						for (int i = 0; i < payload; i++)
							ping[i] ^= mask[i & 3];
					}
				}
				WS_WRITE(s, ping, payload, 0, 0xA);
				continue;
			}
			case 0xA: // pong
			{
				uint8_t ping[125];
				if (payload)
				{
					int r = TCP_READ(s, ping, payload);
					if (r <= 0)
						return r;
				}
				continue;
			}

			default:
				return -1;
		}		

		int r = TCP_READ(s, buf, payload);
		if (r <= 0)
			return r;

		if (mask)
		{
			for (int i = 0; i < payload; i++)
				buf[i] ^= mask[i & 3];
		}

		buf += payload;
		size -= payload;
		tot_data += payload;

	} while (!(frame[0] & 0x80)); //(FIN bit is not set)

	return tot_data;
}
