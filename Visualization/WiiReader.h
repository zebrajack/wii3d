using namespace System;
using namespace System::Text;
using namespace System::IO;
using namespace System::Net;
using namespace System::Net::Sockets;
using namespace System::Collections;

struct IRState {
	bool found;
	int x;
	int y;
	int size;
};

ref class WiiReader
{
private:
	Socket^ s;
	array<Byte>^ bytesReceived;
	array<Byte>^ cam1Buf;
	array<Byte>^ cam2Buf;
	//array<Byte>^ SocketSendReceive( String^ server, int port )
	//{
	//	array<Byte>^ bytesReceived = gcnew array<Byte>(64);

	//	// Create a socket connection with the specified server and port.
	//	Socket^ s = ConnectSocket( server, port );
	//	if ( s == nullptr )
	//		return nullptr;

	//	// Receive the server home page content.
	//	int nbytes = 0;
	//	do
	//	{
	//		nbytes = s->Receive( bytesReceived, bytesReceived->Length, static_cast<SocketFlags>(0) );
	//	}
	//	while ( nbytes > 0 );

	//	return bytesReceived;
	//}

	bool ConnectWiiServer( String^ server, int port )
	{
		if(s != nullptr && s->Connected)
		{
			s->Disconnect(true);
		}
		s = nullptr;
		IPHostEntry^ hostEntry = nullptr;

		// Get host related information.
		hostEntry = Dns::Resolve( server );

		// Loop through the AddressList to obtain the supported AddressFamily. This is to avoid
		// an exception that occurs when the host IP Address is not compatible with the address family
		// (typical in the IPv6 case).
		IEnumerator^ myEnum = hostEntry->AddressList->GetEnumerator();
		while ( myEnum->MoveNext() )
		{
			IPAddress^ address = safe_cast<IPAddress^>(myEnum->Current);
			IPEndPoint^ endPoint = gcnew IPEndPoint( address,port );
			Socket^ tmpS = gcnew Socket( endPoint->AddressFamily,SocketType::Stream,ProtocolType::Tcp );
			tmpS->Connect( endPoint );
			if ( tmpS->Connected )
			{
				s = tmpS;
				return true;
			}
			else
			{
				continue;
			}
		}
		return false;
	}

	void ParsePacket(IRState* cam, array<Byte>^ packet)
	{
		int pos = 2;
		for(int i = 0; i < 4; i++)
		{
			cam[i].found = BitConverter::ToBoolean(packet, pos);
			pos += 1;
			cam[i].x = BitConverter::ToInt32(packet, pos);
			pos += 4;
			cam[i].y = BitConverter::ToInt32(packet, pos);
			pos += 4;
			cam[i].size = BitConverter::ToInt32(packet, pos);
			pos += 4;
		}
	}

public:
	WiiReader()
	{
		bytesReceived = gcnew array<Byte>(1024);
		cam1Buf = gcnew array<Byte>(64);
		cam2Buf = gcnew array<Byte>(64);
	}

	~WiiReader()
	{

	}

	bool Start()
	{
		// Create a socket connection with the specified server and port.
		if(!ConnectWiiServer("127.0.0.1", 6464))
			return false;
		s->BeginReceive( bytesReceived, 0, 64, SocketFlags::None, gcnew AsyncCallback(this, &WiiReader::ReadData), 0);
		return true;
	}

	void stop()
	{
		s->Disconnect(true);
	}

	void ReadData( IAsyncResult^ ar )
	{
		int read = s->EndReceive(ar);

		if ( read > 0 )
		{
			System::Threading::Monitor::Enter(this);
			try
			{
				MemoryStream^ m = gcnew System::IO::MemoryStream(bytesReceived);
				if(bytesReceived[0] == 1)
				{
					m->Read(cam1Buf, 0, 64);
				}
				else
				{
					m->Read(cam2Buf, 0, 64);
				}
			}
			finally
			{
				System::Threading::Monitor::Exit(this);
			}
		}
		if(s->Connected)
		{
			s->BeginReceive( bytesReceived, 0, 64, SocketFlags::None, gcnew AsyncCallback(this, &WiiReader::ReadData), 0);
		}
		//else
		//{
		//	s->Close();
		//}
	}

//	void rc( IAsyncResult^ ar )
//{
//   //StateObject^ so = safe_cast<StateObject^>(ar->AsyncState);
//   //Socket^ s = so->workSocket;
//
//   int read = s->EndReceive( ar );
//
//   if ( read > 0 )
//   {
//	   System::AsyncCallback^ a = gcnew System::AsyncCallback(this, &WiiReader::rc );
//      //so->sb->Append( Encoding::ASCII->GetString( so->buffer, 0, read ) );
//      s->BeginReceive( bytesReceived, 0, 64, SocketFlags::None,
//		  a , 0 );
//   }
//   else
//   {
//
//      s->Close();
//   }
//}


	void GetWiiData(IRState* cam1, IRState* cam2)
	{
		System::Threading::Monitor::Enter(this);
		try
		{
			ParsePacket(cam1, cam1Buf);
			ParsePacket(cam2, cam2Buf);
		}
		finally
		{
			System::Threading::Monitor::Exit(this);
		}
	}
};

ref class WiiReaderWrapper
{
public:
	static WiiReader^ myReader;
};