#include "..\common\GlbVars.h"
#include "..\common\CommonHelper.h"
#include "RaundProcess.h"
#include "helper.h"
#include "ClReceiveRequestThread.h"
#include "SendRequestThread.h"

#pragma comment(lib, "Rpcrt4.lib")

DWORD WINAPI RequestHashFromSolver( void* context );
uint16_t GetForwardPort( );
void ClReceiveRequestThread( void* pCtx );

size_t min_pkt_len = 0;
SOCKET clSendSocket = INVALID_SOCKET;
SOCKET clRecvSocket = INVALID_SOCKET;
int64_t countMembers = 0;
int64_t delta_tTime = 0;
bool  first_raund;

DWORD WINAPI RequestHashFromSolver( void* context )
{
    ...
    return 0;
}

uint16_t GetForwardPort( )
{
    ...
    free( data_rcv );
    return  GetForwardPort( );
}

void ClReceiveRequestThread( void* pCtx )
{
    int sockaddr_size = sizeof( sockaddr_in );
    countMembers = 0;
    memset( &scMain_ip , 0 , sock_sz );
    size_t header_size = sizeof( Header );
    size_t cm_header_size = sizeof( CmHeader );
    size_t b_cm_data_off = header_size;
    size_t min_pkt_len = sizeof( CmHeader ) + sizeof( uint16_t );
    size_t w_len = sizeof( uint16_t );
    timer_context* tm_ctx = new  timer_context;
    if(tm_ctx == NULL)
    {
        DbgLog( _PREF_ "timer context - ERROR_INSUFFICIENT_BUFFER" );
        closesocket( clRecvSocket );
        WSACleanup( );
        goto end_thread;
    }
    tm_ctx->host_socket = clRecvSocket;
    tm_ctx->sn_host_addr = &csPC_addr;
    tm_ctx->rsv_host_addr = &resv_addr;
    tm_ctx->tmSync_addr = &sgsPC_addr;
    tm_ctx->hTimerQueue = CreateTimerQueue( );
    tm_ctx->p2p_cells = NULL;
    tm_ctx->cs = &cs_pool_queue;
    tm_ctx->TimerCallBack = (WAITORTIMERCALLBACK)&TimeSynchro;
    tm_ctx->tm_period = 0;
    StartTimer( tm_ctx );
   
    int client_addr_size = sizeof( sockaddr_in );
    fd_set s_set;
    timeval timeout = { 5,000000 };
    first_raund = false;
    int cn_peeng = 0;
	
    for(;;)
    {
        DWORD rez = WaitForSingleObject( ev_recv_thread_stop , 0 );
        if(rez == WAIT_OBJECT_0)
        {
            break;
        }

        FD_ZERO( &s_set );
        FD_SET( clRecvSocket , &s_set );

        select( 0 , &s_set , 0 , 0 , &timeout );

        if (FD_ISSET( clRecvSocket , &s_set ))
        {
            sockaddr_in client_addr;

            memset( &client_addr , 0 , sizeof( sockaddr_in ) );
			

            char *data_rcv = (char*)malloc( buff_work_len );
            IsValidPoint( " main receiv buffer: " , data_rcv , NULL , NULL )

            int bsize = SOCKET_ERROR;

	
            bsize = recvfrom( clRecvSocket , data_rcv , buff_work_len , 0 , (sockaddr *)&client_addr , &client_addr_size );
	
            if(bsize == -1)
            {
                free( data_rcv );
                int rez = WSAGetLastError( );
                if(rez == WSAEWOULDBLOCK)
                {
                    DbgLog( _PREF_ "receive recvfrom(clRecvSocket) error: %d , main loop continue " , rez );
                    continue;
                }
                DbgLog( _PREF_ "receive recvfrom(resvSock) error: %d" , rez );
                closesocket( clRecvSocket );
                clRecvSocket = OpenSocket( &resv_addr );
                if(clRecvSocket == INVALID_SOCKET)
                {
                    DbgLog( _PREF_ "Critical receive OpenSocket( clRecvSocket ) error: %d" , WSAGetLastError( ) );
                    break;
                }
                FD_CLR( clRecvSocket , &s_set );
                continue;
            }
	
            if(bsize == 0)
            {
                free( data_rcv );
                continue;
            }
			
            sockaddr_in real_addr( client_addr );
            ReceiveData *recv_data = (ReceiveData *)data_rcv;
            uint16_t synchro = recv_data->head.key;
            uint16_t port_rcv = recv_data->head.ext_port_rcv;
            CommandData * cmData = (CommandData *)recv_data->data;
            uint8_t cmd = cmData->head.cmd;
            uint16_t receiveFromPort = htons( client_addr.sin_port );
            std::string receive_from_ip = inet_ntoa_s( client_addr.sin_addr );
            std::string receive_port = std::to_string( receiveFromPort );
            DbgLog( _PREF_ "Data received from cell IP- %s ; cell PORT- %s" , receive_from_ip.c_str( ) , receive_port.c_str( ) );

		
            if (synchro != SYNCWORD)
            {
                free( data_rcv );
                ShowLog( _PREF_ "ERRORE SEND_SYNHRO (recvfrom( sgHostSocket)): 0x%x\n" , synchro );
                FD_CLR( clRecvSocket , &s_set );
                continue;
            }

            if (port_rcv == INVALID_SOCKET && cmd != GET_FRD_PORT)
            {
                ShowLog( _PREF_ " OUT OFF FORWARD PORT " );
                free( data_rcv );
                FD_CLR( clRecvSocket , &s_set );
                continue;
            }
            uint8_t version = recv_data->head.ver;
            uint8_t direction = recv_data->head.dir;
            int16_t size_packet = recv_data->head.datalen;
            if (size_packet < min_pkt_len)
            {
                ShowLog( _PREF_ "receiver error size_packet: %d" , size_packet );
                free( data_rcv );
                FD_CLR( clRecvSocket , &s_set );
                continue;
            }
			

            uint16_t crc16_ctrl = Get_CRC16( cmData , size_packet );
            if (crc16_ctrl != 0)
            {
                ShowLog( _PREF_ "error CRC16: 0x%x" , crc16_ctrl );
                free( data_rcv );
                FD_CLR( clRecvSocket , &s_set );
                continue;
            }

            DbgLog( " ,command value 0x%x" , cmd );

            if (cmd != GET_FRD_PORT)
            {
                client_addr.sin_port = recv_data->head.ext_port_rcv;
            }

            int16_t pkt_len = 0;

            int8_t *data_rpl = (int8_t *)malloc( buff_work_len );
            if (data_rpl == NULL)
            {
                ShowLog( _PREF_" send buffer for forwarding port: ERROR " );
                closesocket( clSendSocket );
                WSACleanup( );
                goto end_thread;
            }

            SendData* data_reply = (SendData *)data_rpl;

            IsValidPoint( " main send  buffer: " , data_reply , data_rcv , NULL )

			switch(cmd)
			{				
				
				case HI:
				{
					
					switch (direction)
					{
						
						case REQUEST:
						{
							sockaddr_in ctrl_cell( client_addr );
							ctrl_cell.sin_port = port_rcv;
							int16_t findItem = FindIPparamItem( srv_p2p_cells , &ctrl_cell );
							if(findItem < 0)
							{
								sockaddr_in *sa_cell = (sockaddr_in *)malloc( sizeof( sockaddr_in ) );
								if(sa_cell == NULL)
								{
									DbgLog( _PREF_ "receive APPEND_NEW_CELL  - ERROR_INSUFFICIENT_BUFFER" );
									closesocket( clSendSocket );
									WSACleanup( );
									goto end_thread;
								}
								memset( sa_cell , 0 , sock_sz );
								memcpy( sa_cell , &ctrl_cell , sock_sz );

								ShowLog( _PREF_ "receive REQUEST [HI]. Set new IP param in server IP table" );

								EnterCriticalSection( &cs_pool_queue );
								srv_p2p_cells.push_back( sa_cell );
								LeaveCriticalSection( &cs_pool_queue );
							}
							else
							{

								DbgLog( _PREF_ "The IP parameter is already present in the server IP table" );

							}
							pkt_len = BuildReplayPacket( HI , data_reply , NULL , &cs_pool_queue );

						}
						break;
			
						case REPLAY:
						{
							DbgLog( _PREF_ "receive REPLY [HI]" );
							if(start_mode)
							{

								SetEvent( ev_data_send [ 1 ] );
							}

						}
						free( data_rcv );
						free( data_rpl );
						FD_CLR( clRecvSocket , &s_set );
						continue;
						case SYNCH_TIME: 
						{
							char *data = cmData->data;
							if(cmData->head.coutn_param == 0)
							{
								  free( data_rpl );
								free( data_rcv );
								FD_CLR( clRecvSocket , &s_set );
								continue;
							}
							int16_t len_param = *(int16_t*)data;
							data += sizeof( int16_t );
							int64_t server_time;
							memcpy(&server_time , data , len_param);
							ShowLog( " [REPLY HI - SYNCH_TIME],tm- %I64d",server_time);
							__time64_t t_time = time( 0 );
							int64_t tTime( t_time );
							delta_tTime = tTime - server_time;
						}
						continue;
					}
				}
				break;
				
				
				case BYE:
				{
					if(direction == SEND_ALL_TRUST)
					{
						sockaddr_in *sock = (sockaddr_in *)malloc( sizeof( sockaddr_in ) );
						if(sock == NULL)
						{
							free( data_rcv );
							free( data_rpl );
							ShowLog( _PREF_ "remove  ip BYE error  - ERROR_INSUFFICIENT_BUFFER" );
							closesocket( clSendSocket );
							WSACleanup( );
							goto end_thread;
						}
						
						char *data = cmData->data;
						int16_t len_param = *(int16_t*)data;
						if(len_param != sock_sz)
						{
							free( data_rcv );
							free( data_rpl );
							free( sock );
							FD_CLR( clRecvSocket , &s_set );
							continue;
						}
						data += sizeof( int16_t );
						memcpy( sock , data , len_param );

						int16_t findItem = FindIPparamItem( srv_p2p_cells , sock );
						if(findItem >= 0)
						{
							srv_p2p_cells.erase( srv_p2p_cells.begin( ) + findItem );
							countMembers = srv_p2p_cells.size( );
							std::string sender_ip = inet_ntoa_s( client_addr.sin_addr );
							uint16_t svrRecvPort = htons( client_addr.sin_port );
							std::string sender_port = std::to_string( svrRecvPort );
							ShowLog( _PREF_ "receive BYE from cell  IP- %s ,cell port receve- %s" , sender_ip.c_str( ) , sender_port.c_str( ) );
							ShowLog( _PREF_ "Remove IP param from  SgServer IP table" );
							SlRemoveFromTrusts( solverId , sock );

						}

						free( sock );
					}
					free( data_rcv );
					free( data_rpl );
					FD_CLR( clRecvSocket , &s_set );
				}
				continue;
				
				
				case REQUEST_IP_LIST:
				{
				   ...
				}
				continue;
			
				case REGISTRY_MAIN_CELL:
				{
					...
				}
				continue;
	
				case BEGIN_RAUND:
				{
					...
				}
				continue;

				case APPEND_CELL:
				{
					std::string sender_ip = inet_ntoa_s( client_addr.sin_addr );
					char *data = cmData->data;
					int16_t len_param = *(int16_t*)data;
					data += sizeof( int16_t );
					sockaddr_in *sa_cell = (sockaddr_in*)malloc( sizeof( sockaddr_in ) );
					if(sa_cell == NULL)
					{
						DbgLog( _PREF_ "receive APPEND_NEW_CELL  - ERROR_INSUFFICIENT_BUFFER" );
						closesocket( clSendSocket );
						WSACleanup( );
						goto end_thread;
					}
					memcpy( sa_cell , data , len_param );
					std::string ex_cell_ip = inet_ntoa_s( sa_cell->sin_addr );
					uint16_t ex_cellRcvPort = htons( sa_cell->sin_port );
					std::string recv_port = std::to_string( ex_cellRcvPort );
					sa_cell->sin_port = htons( ex_cellRcvPort );
					DbgLog( _PREF_ "receive APPEND_NEW_CELL - OK " );

					int64_t findItem;
					findItem = FindIPparamItem( srv_p2p_cells , sa_cell );
					if(findItem < 0)
					{
						srv_p2p_cells.push_back( sa_cell );
					}
					else
					{
						ShowLog( _PREF_ " WARNING!!!Cell was return after time-out IP- %s, cell port receive- %s" , ex_cell_ip.c_str( ) , recv_port.c_str( ) );
						free( sa_cell );
					}

				}

				free( data_rcv );
				free( data_rpl );
				FD_CLR( clRecvSocket , &s_set );
				continue;
				
				case REMOVE_CELL:
				{
					...
				}
				break;
				
				case UPDATE_IP_TABLE:
				{
					...
				}
				break;
				
				case REQUEST_HASH:

				if (direction == REQUEST)
				{
					...
				}
				continue;
				
				case SEND_READY_DATA: 
				{
						switch(type_data)
						{
							case  HASH_ID: 
							{
								...
							}
							continue;
							
							case TRANS_T: 
							{
								...
							}
							break;							
							
							
							case HASH_RqP:
							{
								...
							}
							break;
							
							case SEQ_RpP:
							{
								...
							}
							break;
			
							case TIME_RpP: 
							{
								...
							}
							break;
													
							case HASH_RpP:
							{
								...
							}
							break;
							
							case TRANS_RpP:
							{
								...
							}
							break;
							
							default:
							break;

						}
						if (type_data == HASH_RqP)
						{
							break;
						}

					}
					free( pFree );

					if(data_TA != NULL)  
					{
						....
					}
					if(peerTA != NULL)
					{
						....
					}
					else
					{
						if(cn_pool_hash ==true && cn_pool_time ==true  && peerTA == NULL)
						{
							...
						}
					}
				}
				free( data_rcv );
				free( data_rpl );
				continue;
				default:
				DbgLog( _PREF_ " !!! DEFAULT enter " );
				free( data_rcv );
				free( data_rpl );
				FD_CLR( clRecvSocket , &s_set );
				continue;
			}
			
            if (pkt_len < (int)min_pkt_len)
            {
                ShowLog( _PREF_ " receiver error pkt_len- %d; comman receive - 0x%x" , pkt_len , cmd );
                free( data_rcv );
                free( data_rpl );
                FD_CLR( clRecvSocket , &s_set );
                continue;
            }

            sockaddr_in *send2_addr = &client_addr;
			

            SendTo( clSendSocket , &csPC_addr , send2_addr , (char*)data_reply , pkt_len , &cs_pool_queue );
			
            free( data_rcv );
            free( data_rpl );
			
            FD_CLR( clRecvSocket , &s_set );
        }
        else
        {
            if (start_mode)
            {
                int8_t *data = (int8_t *)malloc( buff_work_len );
                if(data == NULL)
                {
                    ShowLog( _PREF_" send buffer for forwarding port: ERROR " );
                    closesocket( clSendSocket );
                    WSACleanup( );
                    goto end_thread;
                }

                SendData* data_req = (SendData *)data;

                if(srv_p2p_cells.empty( ) == true)
                {
                    if(ext_recv_port == INVALID_PORT_VALUE)
                    {
                        ext_recv_port = GetForwardPort( );
                    }
                    int16_t pkt_len = BuildReqPacket( HI , data_req , &cs_pool_queue );
                    SendTo( clSendSocket , &csPC_addr , &sgsPC_addr , (char*)data_req , pkt_len , &cs_pool_queue );
                    free( data );
                    ShowLog( _PREF_" enter in TIMEOUT( %d sec, main module loop), no CEELS around " , timeout.tv_sec );
                    continue;
                }

                ShowLog( _PREF_" enter in TIMEOUT( %d sec, main module loop), resend hash ID" , timeout.tv_sec );
                std::vector <sockaddr_in *> ::iterator item;
                for(item = srv_p2p_cells.begin( ); item != srv_p2p_cells.end( ); item++)
                {
                    sockaddr_in* rs_item = *item;
                    sockaddr_in *sa_cell = (sockaddr_in *)malloc( sizeof( sockaddr_in ) );
                    if(sa_cell == NULL)
                    {
                        DbgLog( _PREF_ "REQUEST_HASH  - ERROR_INSUFFICIENT_BUFFER for sockaddr_in cell" );
                        closesocket( clSendSocket );
                        WSACleanup( );
                        goto end_thread;
                    }

                    DbgLog( _PREF_"receive command REQUEST_HASH" );
                    memset( sa_cell , 0 , sock_sz );
                    memcpy( sa_cell , rs_item , sock_sz );
                    bool ret = PingPeer( sa_cell->sin_addr.S_un.S_addr );
                    if(ret == S_FALSE)
                    {
                        continue;
                    }
                    SlGetHashFromCell( solverId , sa_cell );
                }
                int16_t pkt_len = BuildReqPacket( HI , data_req , &cs_pool_queue );
                SendTo( clSendSocket , &csPC_addr , &sgsPC_addr , (char*)data_req , pkt_len , &cs_pool_queue );
                free( data );
            }

            ShowLog( _PREF_" enter in TIMEOUT( %d sec, main module loop), no data receive" , timeout.tv_sec );
        }
    }
end_thread:
    ShowLog( _PREF_"          <End receive thread ok>\n" );
    SetEvent( ev_recv_thread_end );
}
