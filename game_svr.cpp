
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

struct PlayerCon
{
	char* name;
	SOCKET client_socket;
	HANDLE read_thread;
	int index;

	void Start(SOCKET socket)
	{
		client_socket = socket;
		read_thread = CreateThread(0, 0, Recv, this, 0, 0);
	}

	enum CMD
	{
		LOG_IN = 0,
		PLAYER_STATE = 1,
	};

	struct PlayerState
	{
		float pos[3];
		float dir;
		int32_t sprite;
		int32_t anim;
		int32_t frame;
		int32_t flags; // unspecified
	};

	int release_index; // MUST NOT BE USED OUTSIDE OF ACQUIRE/RELEASE
	PlayerState player_state; // this player

	void Recv()
	{
		bool logged = false; // till not logged in only ping is ok

		char buf[2048]; // should be enough for any message size including talkboxes
		int len = 0;
		
		while (1)
		{
			int ret = recv(client_socket, buf + len, 4 - len, 0);
			if (ret <= 0)
			{
				// time to disconnect
				break;
			}

			len += ret;

			if (len >= 4)
			{
				int cmd_size = *(int32_t*)buf;
				if (cmd_size < 0 || cmd_size>2048)
					break;

				if (cmd_size == 4)
				{
					// ping? -> pong
					ret = send(client_socket, (const char*)&cmd_size, 4, 0);

					if (ret != 4)
					{
						// time to disconnect
						break;
					}

					len = 0;
					continue;
				}

				if (cmd_size < 8)
				{
					// invalid!
					// all cmds except ping must contain 4 bytes cmd_id
					// time to disconnect
					break;
				}

				while (len < cmd_size)
				{
					ret = recv(client_socket, buf + len, cmd_size - len, 0);
					if (ret <= 0)
					{
						// time to disconnect
						break;
					}

					len += ret;
				}

				if (ret <= 0)
				{
					// time to disconnect
					break;
				}

				int cmd_id = *(int32_t*)(buf+4);
				void* data = buf + 8;
				len -= 8;
				// ready to parse cmd and dispatch

				if (!logged && cmd_id != LOG_IN || 
					logged && cmd_id == LOG_IN)
				{
					// time to disconnect
					break;
				}

				switch (cmd_id)
				{
					case LOG_IN:
					{
						// check credentials
						// ...

						// if ok, transfer all players & items info!
						// ...

						// order new full snapshot, wait for it and transmit
						// ...

						break;
					}

					case PLAYER_STATE:
					{
						PlayerState* s = (PlayerState*)data;
						memcpy(&player_state, s, sizeof(PlayerState));

						// set entrire row of 10000 bits in dirty matrix (blindly!)
						// so responds for other clients know which states should they include

						// LOCK, copy & clear column, UNLOCK ?

						break;
					}
				}
			}
		}

		// cleanup
	}

	static DWORD WINAPI Recv(LPVOID p)
	{
		PlayerCon* con = (PlayerCon*)p;
		con->Recv();
		return 0;
	}

	static CRITICAL_SECTION cs;
	static const int max_clients = 10000;
	static int clients;
	static int client_id[]; // used first then free
	static PlayerCon players[];

	// identifies which players changed state since last update sent to this player
	uint8_t player_mask[(max_clients + 7) / 8]; // 1.25 KB (12.5 MB total)
	
	// same with all items? -- but what with dynamically created ones (like fruits on / at trees)
	// uint8_t item_mask[(max_items + 7) / 8]]

	// if we process items serially for all players, there would be no problem

	
	static PlayerCon* Aquire() // returns ID
	{
		PlayerCon* con = 0;
		EnterCriticalSection(&cs); // prevent pending releases
		if (clients < max_clients)
		{
			con = players + client_id[clients];
			con->release_index = clients;
			clients++;
		}
		LeaveCriticalSection(&cs);
		return con;
	}

	void Release()
	{
		EnterCriticalSection(&cs); // prevent pending aquire and other releases
		clients--;
		int mov_id = client_id[clients];
		int rel_id = client_id[release_index];
		client_id[release_index] = mov_id;
		client_id[clients] = rel_id;
		players[mov_id].release_index = rel_id;
		LeaveCriticalSection(&cs);
	}
};

CRITICAL_SECTION PlayerCon::cs;
int PlayerCon::clients;
int PlayerCon::client_id[PlayerCon::max_clients]; 
PlayerCon PlayerCon::players[PlayerCon::max_clients];

volatile bool isRunning = true;

BOOL WINAPI consoleHandler(DWORD signal)
{
	switch (signal)
	{
	case CTRL_C_EVENT:
		isRunning = false;
		// Signal is handled - don't pass it on to the next handler
		return TRUE;
	default:
		// Pass signal on to the next handler
		return FALSE;
	}
}

int a3dListen(const char* port)
{
	// setup ^C ?

	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) 
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, port, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) 
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) 
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) 
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// init free ids
	PlayerCon::clients = 0;
	for (int i = 0; i < PlayerCon::max_clients; i++)
		PlayerCon::client_id[i] = i;

	while (isRunning)
	{
		SOCKET ClientSocket = INVALID_SOCKET;

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket != INVALID_SOCKET)
		{
			PlayerCon* con = PlayerCon::Aquire();
			if (!con)
				closesocket(ClientSocket);
			else
				con->Start(ClientSocket);
		}
		else
		{
			if (isRunning)
			{
				printf("accept failed: %d\n", WSAGetLastError());
				isRunning = false;
			}
			else
				printf("[Ctrl]+C\n");

		}
	}

	closesocket(ListenSocket);

	// enum all connections & shut down!
	// ...

	WSACleanup();

	return 0;
}
#else
#endif

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

	a3dListen("8080");

	DeleteWorld(world);
	DeleteTerrain(terrain);

	FreeSprites();

}
