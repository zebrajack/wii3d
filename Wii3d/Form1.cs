using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;
using WiimoteLib;
using DotNetMatrix;

namespace Wii3d
{
    public partial class MainForm : Form
    {
        private delegate void UpdateWiimoteStateDelegate(WiimoteChangedEventArgs args);
        Wiimote wm1 = new Wiimote();
        Wiimote wm2 = new Wiimote();
        Bitmap b1 = new Bitmap(256, 192, PixelFormat.Format24bppRgb);
        Graphics g1;
        Bitmap b2 = new Bitmap(256, 192, PixelFormat.Format24bppRgb);
        Graphics g2;
        float[] points2D = new float[16]; //0~7 is the x and y of 2D points in the first camera [I|0], 8~15 is x and y of 2D points in second camera [R|t]
                                          //*** Important: point2D have to be normalized by multiply K^-1, before 3D reconstruction. ****//
        float [] points3D = new float[12];
        int valid3DPointNumber;
        int [] pairNumber = new int[4]; //p0 in camera1 is corresponds to p0 in camera2

        // Extrinsic parameters of cameras
        float[] cameraRot = new float[9]; //rotation matrix from camera2 to camera1
        float[] cameraTrans = new float[3]; //translation from camera2 to camera1

        // Intrisic parameters of cameras
        float[] K = new float[9];

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            wm1.WiimoteChanged += new WiimoteChangedEventHandler(wm_WiimoteChanged1);
            wm1.Connect(0);
            wm1.SetReportType(Wiimote.InputReport.IRAccel, true);
            wm1.SetLEDs(false, true, true, false);

            wm2.WiimoteChanged += new WiimoteChangedEventHandler(wm_WiimoteChanged2);
            wm2.Connect(1); // skip the first device
            wm2.SetReportType(Wiimote.InputReport.IRAccel, true);
            wm2.SetLEDs(false, true, true, false);
            
            g1 = Graphics.FromImage(b1);
            g2 = Graphics.FromImage(b2);
        }

        private void wm_WiimoteChanged1(object sender, WiimoteChangedEventArgs args)
        {
            BeginInvoke(new UpdateWiimoteStateDelegate(UpdateWiimoteState1), args);
        }

        private void wm_WiimoteChanged2(object sender, WiimoteChangedEventArgs args)
        {
            BeginInvoke(new UpdateWiimoteStateDelegate(UpdateWiimoteState2), args);
        }

        private void CameraCalibration()
        {
            
        }

        private void CrossProduct3( float[] t, float[] R, float [] result)
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

        private void MatrixMulVector(float[] m, float[] v, float[] result)
        {
            result[0] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
            result[1] = m[3] * v[0] + m[4] * v[1] + m[5] * v[2];
            result[2] = m[6] * v[0] + m[7] * v[1] + m[8] * v[2];
        }

        private void Reconstruct3D()
        {
            float[] eMatrix = new float[9]; //essential matrix;
            CrossProduct3(cameraTrans,cameraRot,eMatrix);

            //Normalized 2D points
            float[] normalizedPoints2D = new float[16];
            
            //Normalization
            for (int i = 0; i < 8; i++)
            {
                normalizedPoints2D[2 * i] = (points2D[2 * i] - K[2]) / K[0]; //x
                normalizedPoints2D[2 * i+1] = (points2D[2 * i+1] - K[5]) / K[4]; //y
            }

            //epipolar line: E*x
            float[] eLine = new float[12]; //4 epipolar lines mapped from 4 2D Points in the first camera to second camera
            for (int i=0; i < 4; i++)
            {
                eLine[3 * i] = eMatrix[0] * normalizedPoints2D[2 * i] + eMatrix[1] * normalizedPoints2D[2 * i + 1] + eMatrix[2];
                eLine[3 * i + 1] = eMatrix[3] * normalizedPoints2D[2 * i] + eMatrix[4] * normalizedPoints2D[2 * i + 1] + eMatrix[5];
                eLine[3 * i + 2] = eMatrix[6] * normalizedPoints2D[2 * i] + eMatrix[7] * normalizedPoints2D[2 * i + 1] + eMatrix[8];
            }
            
            //calculate all pair combination and find the smallest engergy of x2'*E*x1 = 0
            float energy = 99999.0f;

            for (int i=4; i<8; i++)
                for (int j = 4; j<8; j++)
                    for (int k = 4; k < 8; k++)
                        for (int l = 4; l < 8; l++)
                        {
                            if (i == j || j == k || k == l || i == k || j == l || i == l)
                                continue;
                            
                            float tEnergy = 0;
                            tEnergy += Math.Abs(normalizedPoints2D[2 * i] * eLine[3 * i] + normalizedPoints2D[2 * i + 1] * eLine[3 * i + 1] + eLine[3 * i + 2]);
                            tEnergy += Math.Abs(normalizedPoints2D[2 * j] * eLine[3 * j] + normalizedPoints2D[2 * j + 1] * eLine[3 * j + 1] + eLine[3 * j + 2]);
                            tEnergy += Math.Abs(normalizedPoints2D[2 * k] * eLine[3 * k] + normalizedPoints2D[2 * k + 1] * eLine[3 * k + 1] + eLine[3 * k + 2]);
                            tEnergy += Math.Abs(normalizedPoints2D[2 * l] * eLine[3 * l] + normalizedPoints2D[2 * l + 1] * eLine[3 * l + 1] + eLine[3 * l + 2]);
                            if (tEnergy < energy)
                            {
                                energy = tEnergy;
                                pairNumber[0] = i;
                                pairNumber[1] = j;
                                pairNumber[2] = k;
                                pairNumber[3] = l;
                            }
                        }


            double[][] A = new double[4][];
            for (int i = 0; i < 4; i++)
                A[i] = new double[4];


            //compute the 3D position according to the points position
            //solve the least square solution of x * (PX) =0
            //Reference: Multiple View Geometry in Computer Vision: p312
            for (int i = 0; i < 4; i++)
            {
                A[i][0] = -1;
                A[i][1] = 0;
                A[i][2] = normalizedPoints2D[2*i];
                A[i][3] = 0;

                A[i][0] = 0;
                A[i][1] = -1;
                A[i][2] = normalizedPoints2D[2 * i+1];
                A[i][3] = 0;

                A[i][0] = -1;
                A[i][1] = 0;
                A[i][2] = normalizedPoints2D[2 * pairNumber[i]];
                A[i][3] = 0;

                A[i][0] = 0;
                A[i][1] = -1;
                A[i][2] = normalizedPoints2D[2 * pairNumber[i]+1];
                A[i][3] = 0;


                GeneralMatrix AG = new GeneralMatrix(A,4, 4);


                GeneralMatrix BG = new GeneralMatrix(4, 1);

                GeneralMatrix XG = new GeneralMatrix(4, 1);

                XG = AG.Solve(BG);

                points3D[i * 3] =  (float) (XG.GetElement(0,0)/ XG.GetElement(3,0));
                points3D[i * 3 + 1] = (float)(XG.GetElement(1, 0) / XG.GetElement(3, 0));
                points3D[i * 3 + 2] = (float)(XG.GetElement(2, 0) / XG.GetElement(3, 0));
            }
            

        }

        private void UpdateWiimoteState1(WiimoteChangedEventArgs args)
        {
            WiimoteState ws = args.WiimoteState;

            if (ws.IRState.Found1)
            {
                lblCam1IR1.Text = ws.IRState.X1.ToString() + ", " + ws.IRState.Y1.ToString() + ", " + ws.IRState.Size1;
                lblCam1IR1Raw.Text = ws.IRState.RawX1.ToString() + ", " + ws.IRState.RawY1.ToString();
                lblCam1IR1.BackColor = Color.Blue;
                lblCam1IR1Raw.BackColor = Color.Blue;
            }
            else
            {
                lblCam1IR1.Text = "NA";
                lblCam1IR1Raw.Text = "NA";
                lblCam1IR1.BackColor = Color.Empty;
                lblCam1IR1Raw.BackColor = Color.Empty;
            }
            if (ws.IRState.Found2)
            {
                lblCam1IR2.Text = ws.IRState.X2.ToString() + ", " + ws.IRState.Y2.ToString() + ", " + ws.IRState.Size2;
                lblCam1IR2Raw.Text = ws.IRState.RawX2.ToString() + ", " + ws.IRState.RawY2.ToString();
                lblCam1IR2.BackColor = Color.Blue;
                lblCam1IR2Raw.BackColor = Color.Blue;
            }
            else
            {
                lblCam1IR2.Text = "NA";
                lblCam1IR2Raw.Text = "NA";
                lblCam1IR2.BackColor = Color.Empty;
                lblCam1IR2Raw.BackColor = Color.Empty;
            }
            if (ws.IRState.Found3)
            {
                lblCam1IR3.Text = ws.IRState.X3.ToString() + ", " + ws.IRState.Y3.ToString() + ", " + ws.IRState.Size3;
                lblCam1IR3Raw.Text = ws.IRState.RawX3.ToString() + ", " + ws.IRState.RawY3.ToString();
                lblCam1IR3.BackColor = Color.Blue;
                lblCam1IR3Raw.BackColor = Color.Blue;
            }
            else
            {
                lblCam1IR3.Text = "NA";
                lblCam1IR3Raw.Text = "NA";
                lblCam1IR3.BackColor = Color.Empty;
                lblCam1IR3Raw.BackColor = Color.Empty;
            }
            if (ws.IRState.Found4)
            {
                lblCam1IR4.Text = ws.IRState.X4.ToString() + ", " + ws.IRState.Y4.ToString() + ", " + ws.IRState.Size4;
                lblCam1IR4Raw.Text = ws.IRState.RawX4.ToString() + ", " + ws.IRState.RawY4.ToString();
                lblCam1IR4.BackColor = Color.Blue;
                lblCam1IR4Raw.BackColor = Color.Blue;
            }
            else
            {
                lblCam1IR4.Text = "NA";
                lblCam1IR4Raw.Text = "NA";
                lblCam1IR4.BackColor = Color.Empty;
                lblCam1IR4Raw.BackColor = Color.Empty;
            }

            g1.Clear(Color.Black);
            if (ws.IRState.Found1)
                g1.DrawEllipse(new Pen(Color.Red), (int)(ws.IRState.RawX1 / 4), (int)(ws.IRState.RawY1 / 4), ws.IRState.Size1 + 1, ws.IRState.Size1 + 1);
            if (ws.IRState.Found2)
                g1.DrawEllipse(new Pen(Color.Blue), (int)(ws.IRState.RawX2 / 4), (int)(ws.IRState.RawY2 / 4), ws.IRState.Size2 + 1, ws.IRState.Size2 + 1);
            if (ws.IRState.Found3)
                g1.DrawEllipse(new Pen(Color.Yellow), (int)(ws.IRState.RawX3 / 4), (int)(ws.IRState.RawY3 / 4), ws.IRState.Size3 + 1, ws.IRState.Size3 + 1);
            if (ws.IRState.Found4)
                g1.DrawEllipse(new Pen(Color.Orange), (int)(ws.IRState.RawX4 / 4), (int)(ws.IRState.RawY4 / 4), ws.IRState.Size4 + 1, ws.IRState.Size4 + 1);
            //if (ws.IRState.Found1 && ws.IRState.Found2)
                //g1.DrawEllipse(new Pen(Color.Green), (int)(ws.IRState.RawMidX / 4), (int)(ws.IRState.RawMidY / 4), 2, 2);
            pbIR1.Image = b1;
        }

        private void UpdateWiimoteState2(WiimoteChangedEventArgs args)
        {
            WiimoteState ws = args.WiimoteState;

            if (ws.IRState.Found1)
            {
                lblCam2IR1.Text = ws.IRState.X1.ToString() + ", " + ws.IRState.Y1.ToString() + ", " + ws.IRState.Size1;
                lblCam2IR1Raw.Text = ws.IRState.RawX1.ToString() + ", " + ws.IRState.RawY1.ToString();
                lblCam2IR1.BackColor = Color.Blue;
                lblCam2IR1Raw.BackColor = Color.Blue;
            }
            else
            {
                lblCam2IR1.Text = "NA";
                lblCam2IR1Raw.Text = "NA";
                lblCam2IR1.BackColor = Color.Empty;
                lblCam2IR1Raw.BackColor = Color.Empty;
            }
            if (ws.IRState.Found2)
            {
                lblCam2IR2.Text = ws.IRState.X2.ToString() + ", " + ws.IRState.Y2.ToString() + ", " + ws.IRState.Size2;
                lblCam2IR2Raw.Text = ws.IRState.RawX2.ToString() + ", " + ws.IRState.RawY2.ToString();
                lblCam2IR2.BackColor = Color.Blue;
                lblCam2IR2Raw.BackColor = Color.Blue;
            }
            else
            {
                lblCam2IR2.Text = "NA";
                lblCam2IR2Raw.Text = "NA";
                lblCam2IR2.BackColor = Color.Empty;
                lblCam2IR2Raw.BackColor = Color.Empty;
            }
            if (ws.IRState.Found3)
            {
                lblCam2IR3.Text = ws.IRState.X3.ToString() + ", " + ws.IRState.Y3.ToString() + ", " + ws.IRState.Size3;
                lblCam2IR3Raw.Text = ws.IRState.RawX3.ToString() + ", " + ws.IRState.RawY3.ToString();
                lblCam2IR3.BackColor = Color.Blue;
                lblCam2IR3Raw.BackColor = Color.Blue;
            }
            else
            {
                lblCam2IR3.Text = "NA";
                lblCam2IR3Raw.Text = "NA";
                lblCam2IR3.BackColor = Color.Empty;
                lblCam2IR3Raw.BackColor = Color.Empty;
            }
            if (ws.IRState.Found4)
            {
                lblCam2IR4.Text = ws.IRState.X4.ToString() + ", " + ws.IRState.Y4.ToString() + ", " + ws.IRState.Size4;
                lblCam2IR4Raw.Text = ws.IRState.RawX4.ToString() + ", " + ws.IRState.RawY4.ToString();
                lblCam2IR4.BackColor = Color.Blue;
                lblCam2IR4Raw.BackColor = Color.Blue;
            }
            else
            {
                lblCam2IR4.Text = "NA";
                lblCam2IR4Raw.Text = "NA";
                lblCam2IR4.BackColor = Color.Empty;
                lblCam2IR4Raw.BackColor = Color.Empty;
            }

            g2.Clear(Color.Black);
            if (ws.IRState.Found1)
                g2.DrawEllipse(new Pen(Color.Red), (int)(ws.IRState.RawX1 / 4), (int)(ws.IRState.RawY1 / 4), ws.IRState.Size1 + 1, ws.IRState.Size1 + 1);
            if (ws.IRState.Found2)
                g2.DrawEllipse(new Pen(Color.Blue), (int)(ws.IRState.RawX2 / 4), (int)(ws.IRState.RawY2 / 4), ws.IRState.Size2 + 1, ws.IRState.Size2 + 1);
            if (ws.IRState.Found3)
                g2.DrawEllipse(new Pen(Color.Yellow), (int)(ws.IRState.RawX3 / 4), (int)(ws.IRState.RawY3 / 4), ws.IRState.Size3 + 1, ws.IRState.Size3 + 1);
            if (ws.IRState.Found4)
                g2.DrawEllipse(new Pen(Color.Orange), (int)(ws.IRState.RawX4 / 4), (int)(ws.IRState.RawY4 / 4), ws.IRState.Size4 + 1, ws.IRState.Size4 + 1);
            if (ws.IRState.Found1 && ws.IRState.Found2)
                g2.DrawEllipse(new Pen(Color.Green), (int)(ws.IRState.RawMidX / 4), (int)(ws.IRState.RawMidY / 4), 2, 2);
            pbIR2.Image = b2;
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            wm1.Disconnect();
            wm2.Disconnect();
        }
    }
}