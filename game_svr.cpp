
#include <stdint.h>
#include <string.h>

#include "terrain.h"
#include "world.h"
#include "render.h"
#include "game.h"
#include "network.h"

#define MAX_CLIENTS 50
Server* server = 0; // this is to fullfil game.cpp externs!

bool Server::Send(const uint8_t* data, int size)
{
	return false;
}

void Server::Proc()
{
}

extern "C" void SHA1(void* data, int len, unsigned char digest[20]);

int Base64Encode(unsigned char* data, int len, char* base64)
{
	static const char chr[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/=";

	int chunks = len / 3, i = 0;
	for (; i < chunks; i++)
	{
		int s = 3 * i;
		int d = 4 * i;

		unsigned char 
			a = data[s + 0], 
			b = data[s + 1], 
			c = data[s + 2];

		base64[d + 0] = chr[a >> 2];
		base64[d + 1] = chr[((a & 0x3) << 4) | (b >> 4)];
		base64[d + 2] = chr[((b & 0xF) << 2) | (c >> 6)];
		base64[d + 3] = chr[c & 0x3F];
	}

	int s = 3 * i;
	if (s<len)
	{
		int d = 4 * i;
		unsigned char a = data[s + 0];
		if (s + 1 >= len)
		{
			base64[d + 0] = chr[a >> 2];
			base64[d + 1] = chr[(a & 0x3) << 4];
			base64[d + 2] = chr[64];
			base64[d + 3] = chr[64];
		}
		else
		if (s + 2 >= len)
		{
			unsigned char b = data[s + 1];
			base64[d + 0] = chr[a >> 2];
			base64[d + 1] = chr[((a & 0x3) << 4) | (b >> 4)];
			base64[d + 2] = chr[(b & 0xF) << 2];
			base64[d + 3] = chr[64];
		}
		return d + 4;
	}

	return 4 * i;
}

struct BroadCast /* rare messages like talk item(add/rem/del) join and exit */
{
	void Send(int id_from);

	volatile unsigned int refs; // interlocked?
	int pad;

	BroadCast* next[MAX_CLIENTS];
	int size;

	// just data to send
	// ...
};

struct PlayerCon
{
	// char* name;
	volatile TCP_SOCKET client_socket;
	RWLOCK_HANDLE* rwlock;

	bool Start(TCP_SOCKET socket)
	{
		client_socket = socket;
		rwlock = RWLOCK_CREATE();
		return THREAD_CREATE_DETACHED(Recv, this);
	}

	void Stop()
	{
		TCP_SOCKET s = client_socket;
		client_socket = INVALID_TCP_SOCKET; // atomic?
		TCP_CLOSE(s);
	}

	struct PlayerState
	{
		float pos[3];
		float dir;
		int32_t am; // action / mount
		int32_t sprite;
		int32_t anim;
		int32_t frame;
		int32_t flags; // unspecified
	};


	PlayerState player_state; // this player

	bool joined;
	bool has_state;
	char player_name[32];

	// handled by every client when it receives 'P'ose request
	BroadCast* head;
	BroadCast* tail;

	int broadcasts; // fuse

	void Recv()
	{
		int ID = (int)(this - players);
		printf("CONNECTED ID: %d\n", ID);
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

		struct Headers
		{
			static int cb(const char* header, const char* value, void* param)
			{
				Headers* h = (Headers*)param;

				int mask = 1;

				if (!header)
				{
					if (h->parsed & mask)
						return -3;
					h->parsed |= mask;

					const char* match = "GET /ws/y4/ HTTP/1.1";

					if (strcmp(value, match) != 0)
						return -3;

					return 0;
				}

				mask <<= 1;

				if (strcmp(header, "Sec-WebSocket-Version") == 0)
				{
					if (h->parsed & mask)
						return -3;
					h->parsed |= mask;
					if (strcmp(value, "13") != 0)
						return -3;
					return 0;
				}

				mask <<= 1;

				if (strcmp(header, "Sec-WebSocket-Key") == 0)
				{
					if (h->parsed & mask)
						return -3;
					h->parsed |= mask;
					h->keylen = (int)strlen(value);
					if (h->keylen >= 64)
						return -3;
					strcpy(h->key, value);
					return 0;
				}

				mask <<= 1;

				if (strcmp(header, "Upgrade") == 0)
				{
					if (h->parsed & mask)
						return -3;
					h->parsed |= mask;
					if (strcmp(value, "WebSocket") != 0 && strcmp(value, "websocket") != 0)
						return -3;
					return 0;
				}

				mask <<= 1;

				if (strcmp(header, "Connection") == 0)
				{
					if (h->parsed & mask)
						return -3;
					h->parsed |= mask;

					// slice by commas
					int i = 0, j = 0;
					while (1)
					{
						if (value[j] == ',' || value[j] == 0)
						{
							if (j - i == 7 && strncmp(value + i, "Upgrade", 7) == 0)
							{
								return 0;
							}

							if (value[j] == 0)
								return -3;

							i = j+1;
							if (value[i] != ' ')
								return -3;
							i++;
							j = i;
						}
						else
							j++;
					}

					return -3;
				}

				mask <<= 1;

				if (strcmp(header, "Content-Length") == 0)
				{
					if (h->parsed & mask)
						return -3;
					h->parsed |= mask;
					if (strcmp(value, "0") != 0)
						return -3;
					return 0;
				}

				return 0;
			}

			int keylen;
			char key[128];
			int parsed;
		} headers;

		headers.parsed = 0;

		int ok = HTTP_READ(client_socket, Headers::cb, &headers, 0);
		if (ok != 0 || (headers.parsed & 31) != 31)
		{
			Release();
			return;
		}

		strcpy(headers.key + headers.keylen, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

		unsigned char digest[20];
		SHA1(headers.key, (int)strlen(headers.key), digest);

		char base64[(20 + 2) / 3 * 4 + 1];
		Base64Encode(digest, 20, base64);
		base64[(20 + 2) / 3 * 4] = 0;

		static const char response_fmt[] = 
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: WebSocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Version: 13\r\n"
			"Sec-WebSocket-Accept: %s\r\n\r\n";

		char response_buf[256];
		int response_len = sprintf(response_buf, response_fmt, base64);

		int w = TCP_WRITE(client_socket, (const uint8_t*)response_buf, response_len);
		if (w <= 0)
		{
			Release();
			return;
		}

		printf("------------- AFTER-SHAKE ----------------\n");
		
		while (1)
		{
			bool bin = false;
			int size = WS_READ(client_socket, buf, 2047, &bin);
			if (size <= 0)
				break;

			//   input messages:
			//   ---------------
			//   JOIN: name_len, {name} -> respond with id and all items in world (FULL ITEM LOCK), then broadcast JOIN to all
			//   POSE: x,y,z,dir,sprite,anim,frame -> respond with all players poses!!! (no broadcast)
			//   ITEM: pick_item(or -1), drop_item(or -1), consume_item(or -1), items_to_lock[10], items_to_unlock[10] (both arrays are -1 terminated but transmitted in full)
			//         -> respond with accumulated_items_locked[10] (-1 terminated, transmitted i ful), {all these items} (no broadcast)
			//   TALK: string length {string} -> (no response) broadcast TALK to all

			//   broadcast responses:
			//   --------------------
			//   JOIN: id, name
			//   EXIT: id, (auto by socket err) all items must be reinserted to world in last know position
			//   TALK: id, string (by id's TALK)
			//   INSERT: item_index, x,y,z (generated by someones drop)
			//   REMOVE: item_index (generated by someones successful pick)
			//   DELETE: item_index (generated by someones consumption)
			//   CREATE: item_index, proto_index, x,y,z (generated by server / game_master)

			switch (buf[0])
			{
				case 'P':
				{
					RWLOCK_WRITE_LOCK(rwlock);

					// handle broadcasts first !
					int num = 0;
					while (head)
					{
						BroadCast* n = head->next[ID];

						int w = WS_WRITE(client_socket, (uint8_t*)(head + 1), head->size, 0, true);
						if (w <= 0)
						{
							RWLOCK_WRITE_UNLOCK(rwlock);
							Release();
							return;
						}

						head->next[ID] = 0;
						if (INTERLOCKED_DEC(&head->refs) == 0)
							free(head);

						head = n;
						num++;
					}

					assert(num == broadcasts);

					tail = 0;
					broadcasts = 0;
					
					if (num)
						printf("ID:%d processed %d broadcasts\n", ID, num);

					STRUCT_REQ_POSE* req_pose = (STRUCT_REQ_POSE*)buf;
					if (size != sizeof(STRUCT_REQ_POSE))
					{
						RWLOCK_WRITE_UNLOCK(rwlock);
						Release();
						printf("DISCONNECTED ID: %d\n", ID);
						return;
					}

					has_state = true;

					if (player_state.pos[0] != req_pose->pos[0] ||
						player_state.pos[1] != req_pose->pos[1] ||
						player_state.pos[2] != req_pose->pos[2] ||
						player_state.dir != req_pose->dir ||
						player_state.am != req_pose->am ||
						player_state.sprite != req_pose->sprite ||
						player_state.anim != req_pose->anim ||
						player_state.frame != req_pose->frame)
					{

						player_state.pos[0] = req_pose->pos[0];
						player_state.pos[1] = req_pose->pos[1];
						player_state.pos[2] = req_pose->pos[2];
						player_state.dir = req_pose->dir;
						player_state.am = req_pose->am;
						player_state.sprite = req_pose->sprite;
						player_state.anim = req_pose->anim;
						player_state.frame = req_pose->frame;

						RWLOCK_WRITE_UNLOCK(rwlock);

						// do it by broadcast
						struct PoseBroadCast : BroadCast, STRUCT_BRC_POSE {} *broadcast =
							(PoseBroadCast*)malloc(sizeof(PoseBroadCast));

						broadcast->size = sizeof(STRUCT_BRC_POSE);
						broadcast->token = 'p';
						broadcast->id = ID;
						broadcast->pos[0] = player_state.pos[0];
						broadcast->pos[1] = player_state.pos[1];
						broadcast->pos[2] = player_state.pos[2];
						broadcast->dir = player_state.dir;
						broadcast->am = player_state.am;
						broadcast->sprite = player_state.sprite;
						broadcast->anim = player_state.anim;
						broadcast->frame = player_state.frame;

						broadcast->Send(ID);
					}
					else
						RWLOCK_WRITE_UNLOCK(rwlock);

					break;
				}
				case 'J':
				{
					if (joined)
					{
						Release();
						return;
					}

					STRUCT_REQ_JOIN* req_join = (STRUCT_REQ_JOIN*)buf;
					if (size != sizeof(STRUCT_REQ_JOIN) || req_join->name[30] != 0)
					{
						Release();
						return;
					}

					RWLOCK_READ_LOCK(cs);

					RWLOCK_WRITE_LOCK(rwlock);
					strcpy(player_name, req_join->name);
					joined = true;
					has_state = false;
					player_state.am = 0;
					player_state.anim = 0;
					player_state.dir = 0;
					player_state.flags = 0;
					player_state.sprite = 0;
					player_state.pos[0] = 0;
					player_state.pos[1] = 0;
					player_state.pos[2] = -1000;
					RWLOCK_WRITE_UNLOCK(rwlock);

					STRUCT_RSP_JOIN rsp_join = { 0 };
					rsp_join.token = 'j';
					rsp_join.maxcli = MAX_CLIENTS;
					rsp_join.id = ID;

					size = WS_WRITE(client_socket, (uint8_t*)&rsp_join, sizeof(STRUCT_RSP_JOIN), 0, true);
					if (size <= 0)
					{
						RWLOCK_READ_UNLOCK(cs);
						Release();
						return;
					}

					// for all clients emu join
					STRUCT_BRC_JOIN brc_join = { 0 };
					brc_join.token = 'j';
					for (int i = 0; i < clients; i++)
					{
						int id = client_id[i];
						if (id == ID)
							continue;

						PlayerCon* con = players + id;
						RWLOCK_READ_LOCK(con->rwlock);
						brc_join.anim = con->player_state.anim;
						brc_join.frame = con->player_state.frame;
						brc_join.am = con->player_state.am;
						brc_join.pos[0] = con->player_state.pos[0];
						brc_join.pos[1] = con->player_state.pos[1];
						brc_join.pos[2] = con->player_state.pos[2];
						brc_join.dir = con->player_state.dir;
						brc_join.sprite = con->player_state.sprite;
						strcpy(brc_join.name, con->player_name);
						RWLOCK_READ_UNLOCK(con->rwlock);
						brc_join.id = id;
						brc_join.name[30] = 0;
						brc_join.name[31] = 0;

						size = WS_WRITE(client_socket, (uint8_t*)&brc_join, sizeof(STRUCT_BRC_JOIN), 0, true);
						if (size <= 0)
						{
							RWLOCK_READ_UNLOCK(cs);
							Release();
							return;
						}
					}

					RWLOCK_READ_UNLOCK(cs);


					printf("%s joined with ID:%d\n", player_name, ID);

					// notify others (our socket can be broken but that's fine
					struct ExitBroadCast : BroadCast, STRUCT_BRC_JOIN {} *broadcast =
						(ExitBroadCast*)malloc(sizeof(ExitBroadCast));
					broadcast->size = sizeof(STRUCT_BRC_JOIN);
					broadcast->token = 'j';

					broadcast->anim = 0;
					broadcast->frame = 0;
					broadcast->am = 0;
					broadcast->pos[0] = 0;
					broadcast->pos[1] = 0;
					broadcast->pos[2] = -1000; // hide under water :)
					broadcast->dir = 0;
					broadcast->sprite = 0;

					broadcast->id = ID;
					strcpy(broadcast->name, player_name);
					broadcast->name[30] = 0;
					broadcast->name[31] = 0;
					broadcast->Send(ID);

					break;
				}

				case 'T':
				{
					STRUCT_REQ_TALK* req_talk = (STRUCT_REQ_TALK*)buf;
					if (size < 4 || size != 4 + req_talk->len)
					{
						Release();
						return;						
					}

					struct TalkBroadCast : BroadCast, STRUCT_BRC_TALK {} *broadcast =
						(TalkBroadCast*)malloc(sizeof(BroadCast) + 4 + req_talk->len);
					broadcast->size = 4 + req_talk->len;
					broadcast->token = 't';

					broadcast->len = req_talk->len;
					broadcast->id = ID;
					memcpy(broadcast->str, req_talk->str, req_talk->len);
					broadcast->Send(ID);
					break;
				}

				default:
				{
					//oops
					TCP_CLOSE(client_socket);
					client_socket = INVALID_TCP_SOCKET;
					Release();
					return;
				}
			}

			/*
			printf("(%d) %*s\n", size, size, buf);
			buf[size] = 0;

			float data[6]; //x,y,z,ang,frm,lag
			sscanf((char*)buf, "[%f,%f,%f,%f,%f,%f]", data + 0, data + 1, data + 2, data + 3, data + 4, data + 5);

			size = 0;
			for (int zombie = 0; zombie < 10; zombie++)
			{
				size += sprintf((char*)buf+size,"[%f,%f,%f,%f,%f,%f]",
					data[0] + rand() % 21 - 10,
					data[1] + rand() % 21 - 10,
					data[2], data[3], data[4], 11.0f);
			}

			int w = WS_WRITE(client_socket, buf, size, 1000, bin);
			if (w <= 0)
				break;
			*/
		}

		Release();
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

	// static MUTEX_HANDLE* cs;
	static RWLOCK_HANDLE* cs;
	static int clients;
	static int client_id[]; // used first then free
	static PlayerCon players[];
	
	static PlayerCon* Aquire() // returns ID
	{
		PlayerCon* con = 0;
		RWLOCK_WRITE_LOCK(cs); // prevent pending releases
		if (clients < MAX_CLIENTS)
		{
			con = players + client_id[clients];
			con->release_index = clients;
			clients++;
		}
		RWLOCK_WRITE_UNLOCK(cs);
		return con;
	}

	void Release()
	{
		if (client_socket != INVALID_TCP_SOCKET)
		{
			TCP_CLOSE(client_socket);
			client_socket = INVALID_TCP_SOCKET;
		}

		int ID = (int)(this - players);

		if (joined)
		{
			// notify others (our socket can be broken but that's fine)
			struct ExitBroadCast : BroadCast, STRUCT_BRC_EXIT {} *broadcast =
				(ExitBroadCast*)malloc(sizeof(ExitBroadCast));
			broadcast->size = sizeof(STRUCT_BRC_EXIT);
			broadcast->token = 'e';
			broadcast->id = ID;
			broadcast->Send(ID);
		}

		// remove as soon as possible
		RWLOCK_WRITE_LOCK(cs); // prevent pending aquire and other releases
		clients--;
		joined = false;
		has_state = false;
		int mov_id = client_id[clients];
		int rel_id = client_id[release_index];
		client_id[release_index] = mov_id;
		client_id[clients] = rel_id;
		players[mov_id].release_index = rel_id;

		// remove broadcasts
		while (head)
		{
			BroadCast* n = head->next[ID];
			head->next[ID] = 0;
			if (INTERLOCKED_DEC(&head->refs) == 0)
				free(head);
			head = n;
		}
		tail = 0;
		broadcasts = 0;

		RWLOCK_DELETE(rwlock);
		rwlock = 0;

		RWLOCK_WRITE_UNLOCK(cs);

		printf("DISCONNECTED ID: %d\n", ID);
	}

	//////////////////////////////////////////////////////
};

void BroadCast::Send(int id_from)
{
	refs = 1;
	memset(next, 0, sizeof(BroadCast*) * MAX_CLIENTS);

	RWLOCK_READ_LOCK(PlayerCon::cs);

	for (int i = 0; i < PlayerCon::clients; i++)
	{
		int id = PlayerCon::client_id[i];
		if (id == id_from)
			continue;

		PlayerCon* con = PlayerCon::players + id;
		RWLOCK_WRITE_LOCK(con->rwlock);
		if (*(uint8_t*)(this + 1) == 'p' && 
			con->broadcasts >= 10 * MAX_CLIENTS) // 500 broadcasts awaiting, make it easier
		{
			RWLOCK_WRITE_UNLOCK(con->rwlock);
		}
		else
		{
			if (con->broadcasts >= 100 * MAX_CLIENTS) // 5000 broadcasts awaiting (looks like client can't handle it)
			{
				if (con->client_socket)
				{
					// nasty!
					TCP_CLOSE(con->client_socket);
					con->client_socket = INVALID_TCP_SOCKET;
				}
			}
			else
			{
				con->broadcasts++;
				if (con->tail)
					con->tail->next[id] = this;
				else
					con->head = this;
				con->tail = this;
				INTERLOCKED_INC(&refs);
			}
			RWLOCK_WRITE_UNLOCK(con->rwlock);
		}
	}

	RWLOCK_READ_UNLOCK(PlayerCon::cs);

	if (INTERLOCKED_DEC(&refs) == 0)
	{
		free(this);
	}
}


RWLOCK_HANDLE* PlayerCon::cs;
int PlayerCon::clients;
int PlayerCon::client_id[MAX_CLIENTS];
PlayerCon PlayerCon::players[MAX_CLIENTS];

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
	struct addrinfo *result = NULL, hints;

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
	memset(PlayerCon::players, 0, sizeof(PlayerCon) * MAX_CLIENTS);
	PlayerCon::clients = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
		PlayerCon::client_id[i] = i;

	PlayerCon::cs = RWLOCK_CREATE();

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
			if (!con->Start(ClientSocket))
			{
				TCP_CLOSE(ClientSocket);
				ClientSocket = INVALID_TCP_SOCKET;
				con->Release();
			}
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

	for (int i = 0; i < PlayerCon::clients; i++)
	{
		int id = PlayerCon::client_id[i];
		PlayerCon* con = PlayerCon::players + id;
		con->Stop();
	}

	RWLOCK_DELETE(PlayerCon::cs);

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
