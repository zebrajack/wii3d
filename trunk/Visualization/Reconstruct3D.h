//Code for 3D reconstruction
#ifndef _RECONSTRUCT3D_
#define _RECONSTRUCT3D_

#include <math.h>
#include <cv.h>
#include "linear/jama_qr.h"

#define THRESHHOLD 20

float points2D[16]; //0~7 is the x and y of 2D points in the first camera [I|0], 8~15 is x and y of 2D points in second camera [R|t]
                                  //*** Important: points2D have to be normalized by multiply K^-1, before 3D reconstruction. ****//
float points3D[12];

float oldPoints3D[12]; //to take advantage of time coherence, when the case that more than one points are very close to the epipolar line.

// Extrinsic parameters of cameras
float cameraRot[9]; //rotation matrix from camera2 to camera1
float cameraTrans[3]; //translation from camera2 to camera1
float cameraTrans1[3] ;//translation of first camera, will be used to transform world coordinate back
float cameraRot1[9]; //rotation of first camera

// Intrisic parameters of cameras
float K1[9], K2[9];

//int valid3DPointNumber; //if some points has become invisible for a long time, it can not be calculated.
int pairNumber[4]; //p0 in camera1 is corresponds to p0 in camera2
int pairNumber2[4]; //second possible pair number.

bool useTemporal = false;

bool calibrated = false;

void Calibration()
{
	//sort points2D to check left upper, left down, right upper, right down points
	//initialized
	int l1=points2D[0]<points2D[2]?0:1;
	int l2 =1-l1;
	int d1 = points2D[1]<points2D[3]?0:1;
	int d2 = 1-d1;
	float lsecmin = max(points2D[0],points2D[2]);
	float dsecmin = max(points2D[1],points2D[3]);

	for (int i = 2; i < 4; i++ )
	{
		if (points2D[2 * i] < lsecmin)
		{
			if (points2D[2 * l1] < points2D[2 * i])
			{
				l2 = i;
			}
			else
			{
				l2 = l1;
				l1 = i;

			}
			lsecmin = points2D[2 * l2];
		}

		if (points2D[2 * i + 1] < dsecmin)
		{
			if (points2D[2 * d1+1] < points2D[2 * i + 1])
			{
				d2 = i;
			}
			else
			{
				d2 = d1;
				d1 = i;
			}
			dsecmin = points2D[2 * d2+1];
		}
	}

	//assign 3d coordinates for the 4 points
	float x, y, z;
	float p3[12];
	for (int i = 0; i < 4; i++)
	{
		
		if (l1 == i || l2 == i)
			x = -6;
		else
			x = 6;
		if (d1 == i || d2 == i)
			y = -6;
		else
			y = 6;

		p3[3*i] = x;
		p3[3*i+1] = y;
		p3[3*i+2] = 0;
	}

	//calibration
	int count[] = {4};
	
	CvMat mP42D1 = cvMat(4,2,CV_32FC1,points2D);
	CvMat mP43D1 = cvMat(4,3,CV_32FC1,p3);
	CvMat mCount = cvMat(1,1,CV_32SC1,count);
	CvMat * intrinsicMat1 = cvCreateMat(3,3,CV_32FC1);
	CvMat * distortionCoeff = cvCreateMat(1,4,CV_32FC1);
	CvMat * rotVec1 = cvCreateMat(1,3,CV_32FC1);
	CvMat * transVec1 = cvCreateMat(1,3,CV_32FC1);

	cvCalibrateCamera2(&mP43D1, &mP42D1, &mCount, cvSize(1024,768),intrinsicMat1,distortionCoeff, rotVec1, transVec1);
	for (int i=0; i<9; i++)
		K1[i] = intrinsicMat1->data.fl[i];

	CvMat * rotMat1 = cvCreateMat(3,3,CV_32FC1);
	cvRodrigues2( rotVec1, rotMat1);

	for (int i=0; i<4; i++)
		printf("Point2D%d = %f\t %f\n",i,points2D[2*i],points2D[2*i]+1);

	printf("Intrisic Matrix1 Got from 4 Points:\n");
	for (int i=0; i<3; i++)
	{
		for (int j =0; j<3; j++)
		{
			printf("%f\t", intrinsicMat1->data.fl[i*3+j]);
		}
		printf("\n");
	}

	//Second camera
	l1=points2D[8]<points2D[10]?4:5;
	l2 =9-l1;
	d1 = points2D[9]<points2D[11]?4:5;
	d2 = 9-d1;
	lsecmin = max(points2D[8],points2D[10]);
	dsecmin = max(points2D[9],points2D[11]);

	for (int i = 6; i < 8; i++ )
	{
		if (points2D[2 * i] < lsecmin)
		{
			if (points2D[2 * l1] < points2D[2 * i])
			{
				l2 = i;
			}
			else
			{
				l2 = l1;
				l1 = i;

			}
			lsecmin = points2D[2 * l2];
		}

		if (points2D[2 * i + 1] < dsecmin)
		{
			if (points2D[2 * d1+1] < points2D[2 * i + 1])
			{
				d2 = i;
			}
			else
			{
				d2 = d1;
				d1 = i;
			}
			dsecmin = points2D[2 * d2+1];
		}
	}

	//assign 3d coordinates for the 4 points
	for (int i = 0; i < 4; i++)
	{
		
		if (l1 == i+4 || l2 == i+4)
			x = -6;
		else
			x = 6;
		if (d1 == i+4 || d2 == i+4)
			y = -6;
		else
			y = 6;

		p3[3*i] = x;
		p3[3*i+1] = y;
		p3[3*i+2] = 0;
	}

	CvMat mP43D2 = cvMat(4,3,CV_32FC1,p3);
	CvMat * intrinsicMat2 = cvCreateMat(3,3,CV_32FC1);
	CvMat * rotVec2 = cvCreateMat(1,3,CV_32FC1);
	CvMat * transVec2 = cvCreateMat(1,3,CV_32FC1);

	CvMat mP42D2 = cvMat(4,2,CV_32FC1,points2D+8);
	cvCalibrateCamera2(&mP43D2, &mP42D2, &mCount, cvSize(1024,768),intrinsicMat2,distortionCoeff, rotVec2, transVec2);

	for (int i=0; i<9; i++)
		K2[i] = intrinsicMat2->data.fl[i];

	CvMat * rotMat2 = cvCreateMat(3,3,CV_32FC1);
	cvRodrigues2( rotVec2, rotMat2);
	
	printf("Intrisic Matrix2 Got from 4 Points:\n");
	for (int i=0; i<3; i++)
	{
		for (int j =0; j<3; j++)
		{
			printf("%f\t", intrinsicMat2->data.fl[i*3+j]);
		}
		printf("\n");
	}

	// R = R2*inv(R1), to make the camera1 's rotation matrix as identity
	cameraRot[0] = rotMat1->data.fl[0] * rotMat2->data.fl[0] + rotMat1->data.fl[1] * rotMat2->data.fl[1] + rotMat1->data.fl[2] * rotMat2->data.fl[2];
	cameraRot[1] = rotMat1->data.fl[3] * rotMat2->data.fl[0] + rotMat1->data.fl[4] * rotMat2->data.fl[1] + rotMat1->data.fl[5] * rotMat2->data.fl[2];
	cameraRot[2] = rotMat1->data.fl[6] * rotMat2->data.fl[0] + rotMat1->data.fl[7] * rotMat2->data.fl[1] + rotMat1->data.fl[8] * rotMat2->data.fl[2];
	cameraRot[3] = rotMat1->data.fl[0] * rotMat2->data.fl[3] + rotMat1->data.fl[1] * rotMat2->data.fl[4] + rotMat1->data.fl[2] * rotMat2->data.fl[5];
	cameraRot[4] = rotMat1->data.fl[3] * rotMat2->data.fl[3] + rotMat1->data.fl[4] * rotMat2->data.fl[4] + rotMat1->data.fl[5] * rotMat2->data.fl[5];
	cameraRot[5] = rotMat1->data.fl[6] * rotMat2->data.fl[3] + rotMat1->data.fl[7] * rotMat2->data.fl[4] + rotMat1->data.fl[8] * rotMat2->data.fl[5];	
	cameraRot[6] = rotMat1->data.fl[0] * rotMat2->data.fl[6] + rotMat1->data.fl[1] * rotMat2->data.fl[7] + rotMat1->data.fl[2] * rotMat2->data.fl[8];
	cameraRot[7] = rotMat1->data.fl[3] * rotMat2->data.fl[6] + rotMat1->data.fl[4] * rotMat2->data.fl[7] + rotMat1->data.fl[5] * rotMat2->data.fl[8];
	cameraRot[8] = rotMat1->data.fl[6] * rotMat2->data.fl[6] + rotMat1->data.fl[7] * rotMat2->data.fl[7] + rotMat1->data.fl[8] * rotMat2->data.fl[8];

	printf("R1:\n");
	for (int i=0; i<3; i++)
	{
		for (int j =0; j<3; j++)
		{
			cameraRot1[i*3+j] = rotMat1->data.fl[i*3+j];
			printf("%f\t", rotMat1->data.fl[i*3+j]);
		}
		printf("\n");
	}

	printf("R2:\n");
	for (int i=0; i<3; i++)
	{
		for (int j =0; j<3; j++)
		{
			printf("%f\t", rotMat2->data.fl[i*3+j]);
		}
		printf("\n");
	}
	
	
	printf("R new between Camera2 and Camera1:\n");
	for (int i=0; i<3; i++)
	{
		for (int j =0; j<3; j++)
		{
			printf("%f\t", cameraRot[i*3+j]);
		}
		printf("\n");
	}

	printf("T1:\n");	
	for (int i = 0; i<3; i++)
	{
		cameraTrans1[i] = transVec1->data.fl[i];
		printf("%f\t",transVec1->data.fl[i]);
	}
	printf("\n");

	printf("T2:\n");	
	for (int i = 0; i<3; i++)
	{
		printf("%f\t",transVec2->data.fl[i]);
	}
	printf("\n");

	printf("New Trans:\n");
	// t = t2 - R2*inv(R1)*t1, to make the camera1 stand at 0,0,0
	for (int i=0; i<3; i++)
	{
		cameraTrans[i] = transVec2->data.fl[i] - (cameraRot[3*i]*transVec1->data.fl[0]+cameraRot[3*i+1]*transVec1->data.fl[1]+cameraRot[3*i+2]*transVec1->data.fl[2]);
		printf("%f\t",cameraTrans[i]);
	}
	printf("\n");

	calibrated = true;
}

void CrossProduct3(const float t[], const float R[], float result[])
{
	result[0] = -t[2] * R[3] + t[1] * R[6];
	result[1] = -t[2] * R[4] + t[1] * R[7];
	result[2] = -t[2] * R[5] + t[1] * R[8];
	result[3] = t[2] * R[0] - t[0] * R[6];
	result[4] = t[2] * R[1] - t[0] * R[7];
	result[5] = t[2] * R[2] - t[0] * R[8];
	result[6] = -t[1] * R[0] + t[0] * R[3];
	result[7] = -t[1] * R[1] + t[0] * R[4];
	result[8] = -t[1] * R[2] + t[0] * R[5];
}

void MatrixMulVector3(const float m[], const float v[], float result[])
{
	result[0] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	result[1] = m[3] * v[0] + m[4] * v[1] + m[5] * v[2];
	result[2] = m[6] * v[0] + m[7] * v[1] + m[8] * v[2];
}

void MatrixMulMatrix4x4(const float A[], const float B[], float result[])
{
	result[0] = A[0]*B[0]+A[1]*B[4]+A[2]*B[8]+A[3]*B[12];
	result[1] = A[0]*B[1]+A[1]*B[5]+A[2]*B[9]+A[3]*B[13];
	result[2] = A[0]*B[2]+A[1]*B[6]+A[2]*B[10]+A[3]*B[14];
	result[3] = A[0]*B[3]+A[1]*B[7]+A[2]*B[11]+A[3]*B[15];

	result[4] = A[4]*B[0]+A[5]*B[4]+A[6]*B[8]+A[7]*B[12];
	result[5] = A[4]*B[1]+A[5]*B[5]+A[6]*B[9]+A[7]*B[13];
	result[6] = A[4]*B[2]+A[5]*B[6]+A[6]*B[10]+A[7]*B[14];
	result[7] = A[4]*B[3]+A[5]*B[7]+A[6]*B[11]+A[7]*B[15];

	result[8] = A[8]*B[0]+A[9]*B[4]+A[10]*B[8]+A[11]*B[12];
	result[9] = A[8]*B[1]+A[9]*B[5]+A[10]*B[9]+A[11]*B[13];
	result[10] = A[8]*B[2]+A[9]*B[6]+A[10]*B[10]+A[11]*B[14];
	result[11] = A[8]*B[3]+A[9]*B[7]+A[10]*B[11]+A[11]*B[15];

	result[12] = A[12]*B[0]+A[13]*B[4]+A[14]*B[8]+A[15]*B[12];
	result[13] = A[12]*B[1]+A[13]*B[5]+A[14]*B[9]+A[15]*B[13];
	result[14] = A[12]*B[2]+A[13]*B[6]+A[14]*B[10]+A[15]*B[14];
	result[15] = A[12]*B[3]+A[13]*B[7]+A[14]*B[11]+A[15]*B[15];
}

void MatrixTranspose4(const float A[], float AT[])
{
	AT[0] = A[0];
	AT[1] = A[4];
	AT[2] = A[8];
	AT[3] = A[12];
	AT[4] = A[1];
	AT[5] = A[5];
	AT[6] = A[9];
	AT[7] = A[13];
	AT[8] = A[2];
	AT[9] = A[6];
	AT[10] = A[10];
	AT[11] = A[14];
	AT[12] = A[3];
	AT[13] = A[7];
	AT[14] = A[11];
	AT[15] = A[15];	
}

void Reconstruct3D()
{
	float eMatrix[9]; //essential matrix;
	CrossProduct3(cameraTrans,cameraRot,eMatrix);

	//Normalized 2D points
	float normalizedPoints2D[16];

	//Normalization, confirmed
	for (int i = 0; i < 4; i++)
	{
		normalizedPoints2D[2 * i] = (points2D[2 * i] - K1[2]) / K1[0]; //x
		normalizedPoints2D[2 * i+1] = (points2D[2 * i+1] - K1[5]) / K1[4]; //y
	}

	for (int i = 4; i < 8; i++)
	{
		normalizedPoints2D[2 * i] = (points2D[2 * i] - K2[2]) / K2[0]; //x
		normalizedPoints2D[2 * i+1] = (points2D[2 * i+1] - K2[5]) / K2[4]; //y
	}

	//epipolar line: E*x
	float eLine[12]; //4 epipolar lines mapped from 4 2D Points in the first camera to second camera
	float eLine2[12]; //4 epipolar lines mapped from 4 2D Points in the second camera to first camera

	for (int i=0; i < 4; i++)
	{
		eLine[3 * i] = eMatrix[0] * normalizedPoints2D[2 * i] + eMatrix[1] * normalizedPoints2D[2 * i + 1] + eMatrix[2];
		eLine[3 * i + 1] = eMatrix[3] * normalizedPoints2D[2 * i] + eMatrix[4] * normalizedPoints2D[2 * i + 1] + eMatrix[5];
		eLine[3 * i + 2] = eMatrix[6] * normalizedPoints2D[2 * i] + eMatrix[7] * normalizedPoints2D[2 * i + 1] + eMatrix[8];

		eLine2[3 * i] = eMatrix[0] * normalizedPoints2D[2 * i+8] + eMatrix[1] * normalizedPoints2D[2 * i + 9] + eMatrix[2];
		eLine2[3 * i + 1] = eMatrix[3] * normalizedPoints2D[2 * i+8] + eMatrix[4] * normalizedPoints2D[2 * i + 9] + eMatrix[5];
		eLine2[3 * i + 2] = eMatrix[6] * normalizedPoints2D[2 * i+8] + eMatrix[7] * normalizedPoints2D[2 * i + 9] + eMatrix[8];
	}

	//calculate all pair combination and find the smallest engergy of x2'*E*x1 = 0
	float energy = 99999.0f;

	for (int i=4; i<8; i++)
	{
		for (int j = 4; j<8; j++)
		{
			for (int k = 4; k < 8; k++)
			{
				for (int l = 4; l < 8; l++)
				{
					if (i == j || j == k || k == l || i == k || j == l || i == l)
						continue;

					float tEnergy = 0;
					float e;
					e = normalizedPoints2D[2 * i] * eLine[0] + normalizedPoints2D[2 * i + 1] * eLine[1] + eLine[2];
					tEnergy += abs(e);
					e = normalizedPoints2D[2 * j] * eLine[3] + normalizedPoints2D[2 * j + 1] * eLine[4] + eLine[5];
					tEnergy += abs(e);
					e = normalizedPoints2D[2 * k] * eLine[6] + normalizedPoints2D[2 * k + 1] * eLine[7] + eLine[8];
					tEnergy += abs(e);
					e = normalizedPoints2D[2 * l] * eLine[9] + normalizedPoints2D[2 * l + 1] * eLine[10] + eLine[11];
					tEnergy += abs(e);

					e = normalizedPoints2D[0] * eLine2[(i-4)*3] + normalizedPoints2D[1] * eLine2[(i-4)*3+1] + eLine2[(i-4)*3+2];
					tEnergy += abs(e);
					e = normalizedPoints2D[2] * eLine2[(j-4)*3] + normalizedPoints2D[3] * eLine2[(j-4)*3+1] + eLine2[(j-4)*3+2];
					tEnergy += abs(e);
					e = normalizedPoints2D[4] * eLine2[(k-4)*3] + normalizedPoints2D[5] * eLine2[(k-4)*3+1] + eLine2[(k-4)*3+2];
					tEnergy += abs(e);
					e = normalizedPoints2D[6] * eLine2[(l-4)*3] + normalizedPoints2D[7] * eLine2[(l-4)*3+1] + eLine2[(l-4)*3+2];
					tEnergy += abs(e);

					if (tEnergy < energy)
					{
						energy = tEnergy;
						pairNumber[0] = i;
						pairNumber[1] = j;
						pairNumber[2] = k;
						pairNumber[3] = l;
					}
				}
			}
		}
	}

	if (energy>THRESHHOLD)
	{
		return;
	}

	float  A[12];

	float  B[4];
	float  X[3];

	TNT::Array1D<float> TNT_X = TNT::Array1D<float>(3);

	//compute the 3D position according to the points position
	//solve the least square solution of x cross* (PX) =0
	//Reference: Multiple View Geometry in Computer Vision: p312

	for (int i = 0; i<4; i++)
	{

		A[0] = -1;
		A[1] = 0;
		A[2] = normalizedPoints2D[2*i];

		A[3] = 0;
		A[4] = -1;
		A[5] = normalizedPoints2D[2 * i+1];

		A[6] = normalizedPoints2D[2 * pairNumber[i]]*cameraRot[6]-cameraRot[0];
		A[7] = normalizedPoints2D[2 * pairNumber[i]]*cameraRot[7]-cameraRot[1];
		A[8] = normalizedPoints2D[2 * pairNumber[i]]*cameraRot[8]-cameraRot[2];


		A[9] = normalizedPoints2D[2 * pairNumber[i]+1]*cameraRot[6]-cameraRot[3];
		A[10] = normalizedPoints2D[2 * pairNumber[i]+1]*cameraRot[7]-cameraRot[4];
		A[11] = normalizedPoints2D[2 * pairNumber[i]+1]*cameraRot[8]-cameraRot[5];

		B[0] = 0;
		B[1] = 0;
		B[2] = -normalizedPoints2D[2 * pairNumber[i]]*cameraTrans[2] + cameraTrans[0];
		B[3] = -normalizedPoints2D[2 * pairNumber[i]+1]*cameraTrans[2] + cameraTrans[1];


		TNT::Array2D<float> TNT_A = TNT::Array2D<float>(4,3,A);
		TNT::Array1D<float> TNT_B = TNT::Array1D<float>(4,B);

		JAMA::QR<float> JAMA_QR = JAMA::QR<float>(TNT_A);
		TNT_X = JAMA_QR.solve(TNT_B);

		float p[3];

		if (TNT_X.dim()!=0)
		{
			p[0] =  TNT_X[0];
			p[1] = TNT_X[1];
			p[2] = TNT_X[2];
		}

		for (int j = 0; j<3; j++)
			p[j] -=cameraTrans1[j];

		oldPoints3D[3*i] = points3D[3*i];
		oldPoints3D[3*i] = points3D[3*i];
		oldPoints3D[3*i] = points3D[3*i];

		points3D[3*i] = cameraRot1[0]*p[0] + cameraRot1[3]*p[1] + cameraRot1[6]*p[2];
		points3D[3*i+1] = cameraRot1[1]*p[0] + cameraRot1[4]*p[1] + cameraRot1[7]*p[2];
		points3D[3*i+2] = cameraRot1[2]*p[0] + cameraRot1[5]*p[1] + cameraRot1[8]*p[2];

		//printf("Point%d2D = %f\t %f\n",i, points2D[2*i],points2D[2*i+1]);
		printf("%f\t %f\t %f\n",points3D[3*i],points3D[3*i+1],points3D[3*i+2]);
	}
	if (energy>THRESHHOLD)
	{
		printf("Exists unpaired points, Energy:%f\n.",energy);
		//return;
	}

}

#endif