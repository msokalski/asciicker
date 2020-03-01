
#include <stdint.h>

#include "terrain.h"
#include "world.h"
#include "render.h"
#include "game.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2def.h>
#include <ws2tcpip.h>

#pragma comment(lib,"Ws2_32.lib")

typedef SOCKET TCP_SOCKET;
#define INVALID_TCP_SOCKET INVALID_SOCKET

inline int TCP_INIT()
{
	WSADATA wsaData;
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

inline int TCP_CLOSE(TCP_SOCKET s)
{
	return closesocket(s);
}

inline int TCP_CLEANUP()
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

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h> // compile with -pthread
#include <string.h>

typedef int TCP_SOCKET;
#define INVALID_TCP_SOCKET (-1)

inline int TCP_INIT()
{
	return 0;
}

inline int TCP_CLOSE(TCP_SOCKET s)
{
	return close(s);
}

inline int TCP_CLEANUP()
{
	return 0;
}

struct THREAD_HANDLE
{
	pthread_t th;
};

inline THREAD_HANDLE* THREAD_CREATE(void* (*entry)(void*), void* arg)
{
	pthread_t th;
	pthread_create(&th, 0, entry, arg);
	if (!th)
		return 0;

	THREAD_HANDLE* t = (THREAD_HANDLE*)malloc(sizeof(THREAD_HANDLE));
	t->th = th;
	return t;
}

inline void* THREAD_JOIN(THREAD_HANDLE* thread)
{
	void* ret = 0;
	pthread_join(thread->th, &ret);
	free(thread);
	return ret;
}

struct MUTEX_HANDLE
{
	pthread_mutex_t mu;
};

inline MUTEX_HANDLE* MUTEX_CREATE()
{
	MUTEX_HANDLE* m = (MUTEX_HANDLE*)malloc(sizeof(MUTEX_HANDLE));
	pthread_mutex_init(&m->mu, 0);
	return m;
}

inline void MUTEX_DELETE(MUTEX_HANDLE* mutex)
{
	pthread_mutex_destroy(&mutex->mu);
	free(mutex);
}

inline void MUTEX_LOCK(MUTEX_HANDLE* mutex)
{
	pthread_mutex_lock(&mutex->mu);
}

inline void MUTEX_UNLOCK(MUTEX_HANDLE* mutex)
{
	pthread_mutex_unlock(&mutex->mu);
}

#endif

inline int TCP_WRITE(TCP_SOCKET s, const uint8_t* buf, int size)
{
	return send(s, (const char*)buf, size, 0);
}

inline int TCP_READ(TCP_SOCKET s, uint8_t* buf, int size)
{
	int ret = recv(s, (char*)buf, size, 0);
	for (int i=0; i<ret; i++)
		printf("%c",buf[i]);
	printf("\n");
	return ret;
}

int WS_WRITE(TCP_SOCKET s, const uint8_t* buf, int size, int split)
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
			frame[0] |= 0x2/*BIN*/;

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

		int ofs = 0;

		do
		{
			int w = TCP_WRITE(s, frame + ofs, len - ofs);
			if (w <= 0)
				return w;
			ofs += w;
		} while (ofs < len);

		while (payload)
		{
			int w = TCP_WRITE(s, buf + offs, payload);
			if (w <= 0)
				return w;
			offs += w;
			payload -= w;
		}

	} while (offs < size);

	return size;
}

int WS_READ(TCP_SOCKET s, uint8_t* buf, int size)
{
	if (size < 0)
		return size;

	int len = 0, tot_data = 0;
	uint8_t frame[14];

	do
	{
		// read first 2 bytes
		{
			do
			{
				int r = TCP_READ(s, frame + len, 2-len);
				if (r <= 0)
					return r;
			} while (len < 2);
		}

		uint64_t payload = frame[1] & 0x7F;
		uint8_t* mask = 0;

		if (frame[1] & 0x80) // if mask bit is set
		{

			if (payload == 126)
			{
				// READ next 6 bytes -> first 2 replace payload len, other 4 is xor_mask
				do
				{
					int r = TCP_READ(s, frame + len, 8 - len);
					if (r <= 0)
						return r;
				} while (len < 8);

				payload = (frame[2] << 8) | frame[3];
				mask = frame+4;
			}
			else
			if (payload == 127)
			{
				// READ next 12 bytes -> first 8 replace payload len, other 4 is xor_mask
				do
				{
					int r = recv(s, (char*)frame + len, 14 - len, 0);
					if (r <= 0)
						return r;
				} while (len < 14);

				payload = (frame[2] << 24) | (frame[3] << 16) | (frame[4] << 8) | frame[5];
				payload |= (payload << 32) | (frame[6] << 24) | (frame[7] << 16) | (frame[8] << 8) | frame[9];

				mask = frame + 10;
			}
			else
			{
				// READ next 4 bytes of xor_mask
				do
				{
					int r = recv(s, (char*)frame + len, 6 - len, 0);
					if (r <= 0)
						return r;
				} while (len < 6);

				mask = frame + 2;
			}
		}
		else
		{
			if (payload == 126)
			{
				// READ next 2 bytes->replace payload len
				do
				{
					int r = recv(s, (char*)frame + len, 4 - len, 0);
					if (r <= 0)
						return r;
				} while (len < 4);

				payload = (frame[2] << 8) | frame[3];
			}
			else
			if (payload == 127)
			{
				// READ next 8 bytes->replace payload len
				do
				{
					int r = recv(s, (char*)frame + len, 10 - len, 0);
					if (r <= 0)
						return r;
				} while (len < 10);

				payload = (frame[2] << 24) | (frame[3] << 16) | (frame[4] << 8) | frame[5];
				payload |= (payload << 32) | (frame[6] << 24) | (frame[7] << 16) | (frame[8] << 8) | frame[9];
			}
		}

		if (size < payload)
			return -1;

		int read = (int)payload;
		int offs = 0;

		while (read)
		{
			int r = TCP_READ(s, buf + offs, read);
			if (r <= 0)
				return r;

			if (mask)
			{
				int end = offs + r;
				for (int i = offs; i < end; i++)
					buf[i] ^= mask[i & 3];
			}

			read -= r;
			offs += r;
		}

		buf += offs;
		size -= offs;
		tot_data += offs;

	} while (!(frame[0] & 0x80)); //(FIN bit is not set)

	return tot_data;
}

struct PlayerCon
{
	char* name;
	volatile TCP_SOCKET client_socket;
	THREAD_HANDLE* thread;

	void Start(TCP_SOCKET socket)
	{
		client_socket = socket;

		// is there any old thread hanging around?
		if (thread)
			THREAD_JOIN(thread);

		thread = THREAD_CREATE(Recv, this);
	}

	void Stop()
	{
		TCP_SOCKET s = client_socket;
		client_socket = INVALID_TCP_SOCKET; // atomic?
		TCP_CLOSE(s);
		THREAD_JOIN(thread);
		thread = 0;
	}

	struct PlayerState
	{
		float pos[3];
		float dir;
		int32_t sprite;
		int32_t anim;
		int32_t frame;
		int32_t flags; // unspecified
	};


	PlayerState player_state; // this player

	void Recv()
	{
		printf("CONNECTED ID: %d\n", (int)(this - players));
		uint8_t buf[2048]; // should be enough for any message size including talkboxes

		// read /GET request with some headers, but ensure these: "Upgrade: WebSocket" and "Connection: Upgrade"
		#if 0
		"GET / HTTP/1.1"
		"Host: localhost:8080"
		"User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:73.0) Gecko/20100101 Firefox/73.0"
		"Accept: */*"
		"Accept-Language: en-US,en;q=0.5"
		"Accept-Encoding: gzip, deflate"
		"Sec-WebSocket-Version: 13"
		"Origin: http://asciicker.com"
		"Sec-WebSocket-Extensions: permessage-deflate"
		"Sec-WebSocket-Key: btsPdKGunHdaTPnSSDlfow=="
		"Pragma: no-cache"
		"Cache-Control: no-cache"
		"X-Forwarded-For: 89.64.42.93"
		"X-Forwarded-Host: asciicker.com"
		"X-Forwarded-Server: asciicker.com"
		"Upgrade: WebSocket"
		"Connection: Upgrade"
		#endif

		const int headers = 7;
		int crlf_state=0;
		int header_idx = headers;
		int header_len = 0;
		int value_len = 0;
		int col = 1;
		char header[32];

		static const char* header_match[headers] = 
		{
			"Accept-Encoding",
			"Sec-WebSocket-Extensions", 
			"Sec-WebSocket-Version",	// ensure 13
			"Sec-WebSocket-Key",		// work out new one
			"Upgrade",		 			// ensure WebSocket
			"Connection",				// ensure Upgrade
			"Content-Length"			// ensure none or 0
		};

		char value[headers+1][32] = {0}; // +1 for request line

		do
		{
			int r = TCP_READ(client_socket, buf, 2048);
			if (r<=0)
			{
				Release();
				printf("DISCONNECTED ID: %d\n", (int)(this - players));
				return;
			}

			for (int i=0; i<r; i++)
			{
				switch (buf[i])
				{
					case 0x0D: 
						if (crlf_state == 0 || crlf_state == 2) 
							crlf_state++; 
						else 
							crlf_state = 0; 
						break;

					case 0x0A: 
						if (crlf_state == 1 || crlf_state == 3) 
						{
							if (header_idx>=0)
							{
								if (value_len<32)
									value[header_idx][value_len] = 0;
								else
									value[header_idx][32-1] = 0;
							}
							
							value_len = 0;
							header_len = 0;
							header_idx = -1;
							crlf_state++; 
							col=0;
						}  
						else 
							crlf_state = 0; 
						break;

					default:
					{
						crlf_state = 0;
						if (col==0)
						{
							if (buf[i] == ':')
							{
								col = 1;
								header_idx = -1;
								if (header_len<=32)
								{
									for (int h=0; h<headers; h++)
									{
										if (strncmp(header_match[h],header,header_len)==0)
										{
											header_idx = h;
											break;
										}
									}
								}
							}
							else
							{
								if (header_len < 32)
									header[header_len] = buf[i];
								header_len++;
							}
						}
						else
						{
							if (header_idx>=0)
							{
								if (value_len < 32)
									value[header_idx][value_len] = buf[i];
								value_len++;
							}
						}
						break;
					}
				}

				if (crlf_state == 4)
					break;
			}

		} while (crlf_state != 4);

		// TODO: importand thing is to send back new Sec-WebSocket-Key based on client's one:
		// - first concatenate client's key with this string: "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
		// - then calculate SHA-1 hash of it
		// - finaly resulting Sec-WebSocket-Key is base64 encoding of that hash
		// probably we should also answer back with "Sec-WebSocket-Version: 13"

		static const char response[] = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\n\r\n";
		int w = TCP_WRITE(client_socket, (const uint8_t*)response, sizeof(response)-1);

		printf("------------- AFTER-SHAKE ----------------\n");
		
		while (1)
		{
			TCP_SOCKET s = client_socket; // atomic?
			if (s == INVALID_TCP_SOCKET)
				break;

			int size = WS_READ(s, buf, 2048);

			printf("%d\n", size);

			if (size <= 0)
				break;
		}

		Release();

		printf("DISCONNECTED ID: %d\n", (int)(this - players));
	}

	static void* Recv(void* p)
	{
		PlayerCon* con = (PlayerCon*)p;
		con->Recv();
		return 0;
	}

	//////////////////////////////////////////////////////
	// ID AQUIRE / RELEASE 
	//
	int release_index; // MUST NOT BE USED OUTSIDE OF ACQUIRE/RELEASE

	static MUTEX_HANDLE* cs;
	static const int max_clients = 100;
	static int clients;
	static int client_id[]; // used first then free
	static PlayerCon players[];
	
	static PlayerCon* Aquire() // returns ID
	{
		PlayerCon* con = 0;
		MUTEX_LOCK(cs); // prevent pending releases
		if (clients < max_clients)
		{
			con = players + client_id[clients];
			con->release_index = clients;
			clients++;
		}
		MUTEX_UNLOCK(cs);
		return con;
	}

	void Release()
	{
		MUTEX_LOCK(cs); // prevent pending aquire and other releases
		clients--;
		int mov_id = client_id[clients];
		int rel_id = client_id[release_index];
		client_id[release_index] = mov_id;
		client_id[clients] = rel_id;
		players[mov_id].release_index = rel_id;
		MUTEX_UNLOCK(cs);
	}

	//////////////////////////////////////////////////////
};

MUTEX_HANDLE* PlayerCon::cs;
int PlayerCon::clients;
int PlayerCon::client_id[PlayerCon::max_clients]; 
PlayerCon PlayerCon::players[PlayerCon::max_clients];

volatile bool isRunning = true;

int ServerLoop(const char* port)
{
	int iResult;

	// Initialize Winsock
	iResult = TCP_INIT();
	if (iResult != 0) 
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	TCP_SOCKET ListenSocket = INVALID_TCP_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, port, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		TCP_CLEANUP();
		return 1;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_TCP_SOCKET) 
	{
		//printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		TCP_CLEANUP();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult < 0) 
	{
		//printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		TCP_CLOSE(ListenSocket);
		TCP_CLEANUP();
		return 1;
	}

	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) < 0) 
	{
		//printf("Listen failed with error: %ld\n", WSAGetLastError());
		TCP_CLOSE(ListenSocket);
		TCP_CLEANUP();
		return 1;
	}

	// init, importand to null-out all thread handles
	memset(PlayerCon::players, 0, sizeof(PlayerCon) * PlayerCon::max_clients);
	PlayerCon::clients = 0;
	for (int i = 0; i < PlayerCon::max_clients; i++)
		PlayerCon::client_id[i] = i;

	PlayerCon::cs = MUTEX_CREATE();

	while (isRunning)
	{
		TCP_SOCKET ClientSocket = INVALID_TCP_SOCKET;

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket != INVALID_TCP_SOCKET)
		{
			PlayerCon* con = PlayerCon::Aquire();
			if (!con)
				TCP_CLOSE(ClientSocket);
			else
				con->Start(ClientSocket);
		}
		else
		{
			if (isRunning)
			{
				//printf("accept failed: %d\n", WSAGetLastError());
				isRunning = false;
			}
			else
				printf("[Ctrl]+C\n");

		}
	}

	TCP_CLOSE(ListenSocket);
	MUTEX_DELETE(PlayerCon::cs);

	for (int i = 0; i < PlayerCon::clients; i++)
	{
		int id = PlayerCon::client_id[i];
		PlayerCon* con = PlayerCon::players + id;
		con->Stop();
	}

	TCP_CLEANUP();

	return 0;
}


Terrain* terrain = 0;
World* world = 0;
Material mat[256];
void* GetMaterialArr()
{
	return mat;
}

int main(int argc, char* argv[])
{
	LoadSprites();

	FILE* f = fopen("a3d/game_items.a3d", "rb");

	if (f)
	{
		terrain = LoadTerrain(f);

		if (terrain)
		{
			for (int i = 0; i < 256; i++)
			{
				if (fread(mat[i].shade, 1, sizeof(MatCell) * 4 * 16, f) != sizeof(MatCell) * 4 * 16)
					break;
			}

			world = LoadWorld(f, false);
			if (world)
			{
				// reload meshes too
				Mesh* m = GetFirstMesh(world);

				while (m)
				{
					char mesh_name[256];
					GetMeshName(m, mesh_name, 256);
					char obj_path[4096];
					sprintf(obj_path, "%smeshes/%s", "./"/*root_path*/, mesh_name);
					if (!UpdateMesh(m, obj_path))
					{
						// what now?
						// missing mesh file!
					}

					m = GetNextMesh(m);
				}
			}
		}

		fclose(f);
	}

	// if (!terrain || !world)
	//    return -1;

	// add meshes from library that aren't present in scene file
	char mesh_dirname[4096];
	sprintf(mesh_dirname, "%smeshes", "./"/*root_path*/);
	//a3dListDir(mesh_dirname, MeshScan, mesh_dirname);

	// this is the only case when instances has no valid bboxes yet
	// as meshes weren't present during their creation
	// now meshes are loaded ...
	// so we need to update instance boxes with (,true)

	if (world)
		RebuildWorld(world, true);

	ServerLoop("8080");

	DeleteWorld(world);
	DeleteTerrain(terrain);

	FreeSprites();

}
