/**********************************************************************
*Copyright (c) 2008, Yi GONG,All Rights Reserved.
*DESCRIPTION: Visualization for motion capture
**********************************************************************/
#include <windows.h> 
#include <time.h>
#include <iostream>
#include <gl/glut.h>
#include "bitmap_fonts.h"
#include "reconstruct3D.h"
#using <System.dll>
#include "WiiReader.h"
using namespace System;
using namespace System::Text;
using namespace System::IO;
using namespace System::Net;
using namespace System::Net::Sockets;
using namespace System::Collections;


#define MARKNUM 4

//////////////////////////////////////////////////////////////////////////
//Global parameters
//////////////////////////////////////////////////////////////////////////


//Arcball parameter
bool g_bMouseTracking_Left = false;
int g_iMouseLastPos_Left[2] = {0,0};
float g_fSpin[2]={45.0f,0.0f};
float g_fZ = -250.0f;
float marksPosition[3*MARKNUM];
int g_nWindowWidth = 400;
int g_nWindowHeight = 400;
char infoText[300];





//////////////////////////////////////////////////////////////////////////
void Display();

void Init()
{
	for (int i=0; i<MARKNUM; i++)
	{
		marksPosition[i*3] = i*10+20;
		marksPosition[i*3+1] = i*10-20;
		marksPosition[i*3+2] = i*10+10;
	}

}

void RenderMark(float * markPositionArrays, int markNum)
{
	glColor4f(1.0f,0.0f,1.0f,0.0f);
	glPointSize(15);
	glBegin(GL_POINTS);
	for (int i=0; i<markNum; i++)
	{
		//glPushMatrix();
		//glTranslatef(markPositionArrays[i*3],markPositionArrays[i*3+1],markPositionArrays[i*3+2]);
		//glutWireSphere(0.1,16,16);
		//glPopMatrix();
		glVertex3f(points3D[i*3],points3D[i*3+1],points3D[i*3+2]);
	}

	//render two cameras
	glColor4f(1.0f,0.0f,0.0f,0.0f);
	glVertex3f(0,0,0);
	glVertex3f(cameraTrans[0],cameraTrans[1],cameraTrans[2]);
	glEnd();
}

void RenderCoordinates()
{
	glBegin(GL_LINES);

	glColor4f(1.0f,0.0f,0.0f,0.0f);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(50.0f,0.0f,0.0f);

	glColor4f(0.0f,1.0f,0.0f,0.0f);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(0.0f,50.0f,0.0f);

	glColor4f(0.0f,0.0f,1.0f,0.0f);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(0.0f,0.0f,50.0f);

	glEnd();
}



void Reshape(int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, w/h, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void Idle()
{
	Display();
}

void Keyboard(unsigned char key,int x,int y)
{
	switch (key)
	{

	case 'w':
	case 'W':
		g_fZ +=30;
		break;

	case 's':
	case 'S':
		g_fZ -=30;
		break;

	case 'a':
	case 'A':
		break;

	case 'd':
	case 'D':
		break;

	case 'c':
	case 'C':
		Calibration();
		break;

	}
}

void Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)	
		switch(state)
	{
		case GLUT_DOWN:
			{
				g_iMouseLastPos_Left[0] =  x;
				g_iMouseLastPos_Left[1] =  y;
				g_bMouseTracking_Left = true;
			}
			break;

		case GLUT_UP:
			{
				g_bMouseTracking_Left = false;
			}
			break;
	}

}

void MouseMotion(int x,int y)
{
	if( g_bMouseTracking_Left )
	{
		g_fSpin[0] -= (x - g_iMouseLastPos_Left[0]);
		g_fSpin[1] -= (y - g_iMouseLastPos_Left[1]);

		g_iMouseLastPos_Left[0] = x;
		g_iMouseLastPos_Left[1] = y;

	}

}

Socket^ ConnectSocket( String^ server, int port )
{
	Socket^ s = nullptr;
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
			break;
		}
		else
		{
			continue;
		}
	}
	return s;
}

array<Byte>^ SocketSendReceive( String^ server, int port )
{
	array<Byte>^ bytesReceived = gcnew array<Byte>(2048);

	// Create a socket connection with the specified server and port.
	Socket^ s = ConnectSocket( server, port );
	if ( s == nullptr )
		return nullptr;

	// Receive the server home page content.
	int nbytes = 0;
	do
	{
		nbytes = s->Receive( bytesReceived, bytesReceived->Length, static_cast<SocketFlags>(0) );
	}
	while ( nbytes > 0 );

	return bytesReceived;
}



void Display()
{
	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glPushMatrix();

	//Arcball
	glTranslatef(0.0,0.0,g_fZ);
	glRotatef( -g_fSpin[1], 1.0f, 0.0f, 0.0f );
	glRotatef( -g_fSpin[0], 0.0f, 1.0f, 0.0f );

	RenderCoordinates();
	RenderMark(marksPosition,MARKNUM);
	glPopMatrix();

	for (int i = 0; i<4; i++)
		sprintf( infoText, "P[%d]:%.2f,%.2f,%.2f\n",i,points2D[2*i], points3D[2*i+1],points3D[2*i+2]);

	beginRenderText( g_nWindowWidth, g_nWindowHeight );
	{
		glColor3f( 1.0f, 1.0f, 1.0f );
		renderText( 5, 15, BITMAP_FONT_TYPE_HELVETICA_12, infoText );
	}
	endRenderText();

	glutSwapBuffers();

}

void parsePacket(IRState* cam, array<Byte>^ packet)
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

//void main(int argc, char ** argv)
//{
//	glutInit(&argc, argv);
//	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
//	glutInitWindowSize (g_nWindowWidth, g_nWindowHeight);
//	glutInitWindowPosition (500, 500);
//	glutCreateWindow (argv[0]);
//
//	Init();
//
//	glutDisplayFunc(Display);
//	glutReshapeFunc(Reshape);
//	glutKeyboardFunc(Keyboard);
//	glutMouseFunc(Mouse);
//	glutMotionFunc(MouseMotion);
//	glutIdleFunc(Idle);
//
//	//SocketSendReceive("127.0.0.1", 6464);
//	array<Byte>^ bytesReceived = gcnew array<Byte>(64);
//	IRState* cam1 = new IRState[4];
//	IRState* cam2 = new IRState[4];
//
//	// Create a socket connection with the specified server and port.
//	Socket^ s = ConnectSocket("127.0.0.1", 6464);
//	if ( s == nullptr || s->Connected == false)
//		exit(-1);
//
//	int nbytes = 0;
//
//
//	do
//	{
//
//		nbytes = s->Receive(bytesReceived, 64, static_cast<SocketFlags>(0));
//		if(bytesReceived[0] == 0x01) //camera1
//		{
//			parsePacket(cam1, bytesReceived);
//			valid2DNum[0]=0;
//			for (int i=0; i<4; i++)
//			{
//
//				if (cam1[i].found)
//				{
//					points2D[2*valid2DNum[0]] = cam1[i].x;
//					points2D[2*valid2DNum[0]+1] = cam1[i].y;
//					valid2DNum[0]++;
//				}
//			}
//
//		}
//		else
//		{
//			parsePacket(cam2, bytesReceived);
//			valid2DNum[1]=0;
//			for (int i=0; i<4; i++)
//			{
//
//				if (cam2[i].found)
//				{
//					points2D[2*valid2DNum[1]+8] = cam2[i].x;
//					points2D[2*valid2DNum[1]+9] = cam2[i].y;
//					valid2DNum[1]++;
//				}
//
//			}
//		}
//		if (!calibrated && valid2DNum[0] ==4 &&valid2DNum[1] ==4)
//			Calibration();
//
//		if (calibrated)
//		{
//			Reconstruct3D();
//			Display();
//		}
//
//
//	} while(nbytes > 0 && s->Connected);
//
//	glutMainLoop();
//
//	exit(0);
//
//}

void main(int argc, char ** argv)
{

	WiiReaderWrapper::myReader = gcnew WiiReader;

	if(!WiiReaderWrapper::myReader->Start())
		exit(-1);

	IRState* cam1 = new IRState[4];
	IRState* cam2 = new IRState[4];
	
	WiiReaderWrapper::myReader->GetWiiData(cam1, cam2);
	WiiReaderWrapper::myReader->stop();
	exit(0);

}