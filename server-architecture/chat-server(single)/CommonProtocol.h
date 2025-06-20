enum en_PACKET_TYPE {
	////////////////////////////////////////////////////////
	//
	//	Client & Server Protocol
	//
	////////////////////////////////////////////////////////
	//------------------------------------------------------
	// Chatting Server
	//------------------------------------------------------
	en_PACKET_CS_CHAT_SERVER = 0,
	//------------------------------------------------------------
	// 채팅서버 로그인 요청
	//	{
	//		WORD	Type
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null 포함
	//		WCHAR	Nickname[20]		// null 포함
	//		char	SessionKey[64];		// 인증토큰
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_LOGIN,
	//------------------------------------------------------------
	// 채팅서버 로그인 응답
	//	{
	//		WORD	Type // 2
	//		BYTE	Status				// 0:실패	1:성공 // 1
	//		INT64	AccountNo // 8 11
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_LOGIN,
	//------------------------------------------------------------
	// 채팅서버 섹터 이동 요청
	//	{
	//		WORD	Type
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,
	//------------------------------------------------------------
	// 채팅서버 섹터 이동 결과
	//	{
	//		WORD	Type
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_SECTOR_MOVE,
	//------------------------------------------------------------
	// 채팅서버 채팅보내기 요청
	//	{
	//		WORD	Type
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null 미포함
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_MESSAGE,
	//------------------------------------------------------------
	// 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
	//	{
	//		WORD	Type
	//		INT64	AccountNo
	//		WCHAR	ID[20]						// null 포함
	//		WCHAR	Nickname[20]				// null 포함
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null 미포함
	//	}
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_MESSAGE,
	//------------------------------------------------------------
	// 하트비트
	//	{
	//		WORD		Type
	//	}
	// 클라이언트는 이를 30초마다 보내줌.
	// 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
	//------------------------------------------------------------	
	en_PACKET_CS_CHAT_REQ_HEARTBEAT,

	//------------------------------------------------------
	// Monitor Server  Protocol
	//------------------------------------------------------
	en_PACKET_SS_MONITOR = 20000,
	//------------------------------------------------------
	// Server -> Monitor Protocol
	//------------------------------------------------------
	//------------------------------------------------------------
	// LoginServer, GameServer , ChatServer  가 모니터링 서버에 로그인 함
	//	{
	//		WORD	Type
	//		int		ServerNo		//  각 서버마다 고유 번호를 부여하여 사용
	//	}
	//------------------------------------------------------------
	en_PACKET_SS_MONITOR_LOGIN,
	//------------------------------------------------------------
	// 서버가 모니터링서버로 데이터 전송
	// 각 서버는 자신이 모니터링중인 수치를 1초마다 모니터링 서버로 전송.
	// 서버의 다운 및 기타 이유로 모니터링 데이터가 전달되지 못할떄를 대비하여 TimeStamp 를 전달한다.
	// 이는 모니터링 클라이언트에서 계산,비교 사용한다.
	//	{
	//		WORD	Type
	//		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
	//		int		DataValue				// 해당 데이터 수치.
	//		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)
	//										// 본래 time 함수는 time_t 타입변수이나 64bit 로 낭비스러우니
	//										// int 로 캐스팅하여 전송. 그래서 2038년 까지만 사용가능
	//	}
	//------------------------------------------------------------
	en_PACKET_SS_MONITOR_DATA_UPDATE,
};

enum en_PACKET_SS_MONITOR_DATA_UPDATE {
	dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN = 30,		// 채팅서버 ChatServer 실행 여부 ON / OFF
	dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU = 31,		// 채팅서버 ChatServer CPU 사용률
	dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM = 32,		// 채팅서버 ChatServer 메모리 사용 MByte
	dfMONITOR_DATA_TYPE_CHAT_SESSION = 33,		// 채팅서버 세션 수 (컨넥션 수)
	dfMONITOR_DATA_TYPE_CHAT_PLAYER = 34,		// 채팅서버 인증성공 사용자 수 (실제 접속자)
	dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS = 35,		// 채팅서버 UPDATE 스레드 초당 초리 횟수
	dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL = 36,		// 채팅서버 패킷풀 사용량
	dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL = 37,		// 채팅서버 UPDATE MSG 풀 사용량
};