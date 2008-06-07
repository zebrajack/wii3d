using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;
using WiimoteLib;

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
            if (ws.IRState.Found1 && ws.IRState.Found2)
                g1.DrawEllipse(new Pen(Color.Green), (int)(ws.IRState.RawMidX / 4), (int)(ws.IRState.RawMidY / 4), 2, 2);
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