﻿using System.Diagnostics;
using System.IO.Ports;
using System.Text;
using System.Windows;
using MobileRobotControl.RobotPacket;
using MobileRobotControl.RobotStatusUpdate;
using System.Diagnostics;
using System.Security.Permissions;

namespace MobileRobotControl
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>

    public partial class MainWindow : Window
    {
        private Connection_Window connectionWindow;
        private RS232 rs232;
        private RobotStatusUpdateFactory robotStatusUpdateFactory;
        private PacketSplitter packetSplitter;
        
        delegate void UpdatePosDelegate(float value);
        private UpdatePosDelegate posUpdateDelegate;
        private object delObject;

        delegate void UpdateEngDelegate(int lEngine, int rEngine);
        private UpdateEngDelegate engUpdateDelegate;
        private object[] delObjects = new object[2];

        public MainWindow()
        {
            InitializeComponent();
            robotStatusUpdateFactory = new RobotStatusUpdateFactory();

            robotPosXLabel.Content = sizeof(float).ToString();
            byte[] b = {129, 13, 255, 255, 255};
            var enc = Encoding.GetEncoding("iso-8859-1");
            string a = enc.GetString(b);

            if (a[1] == '\r')
            {
                Debug.WriteLine(a);
            }


            

        }

        private void ConnectMenuItem_Click(object sender, RoutedEventArgs e)
        {
            connectionWindow = new Connection_Window();
            connectionWindow.Owner = this;
            connectionWindow.Port_Open += PortOpenRequest;
            connectionWindow.Show();
        }

        private void PortOpenRequest(SerialPort port)
        {
            try
            {
                rs232.close();
            }
            catch{}

            rs232 = new RS232();
            rs232.PortOpen(port);
            packetSplitter = new PacketSplitter(rs232);
            packetSplitter.PacketReceived += packetReceived;
        }

        private void packetReceived(string data)
        {
            IRobotPacket packet = new RobotPacket.RobotPacket(data);
            IRobotStatusUpdate robotStatusUpdate = robotStatusUpdateFactory.GetRobotStatusUpdate(packet);
            robotStatusUpdate.Execute(this);
        }

#region PositionUpdates
        public void UpdateX(float value)
        {
            if (robotPosXLabel.Dispatcher.CheckAccess())
            {
                robotPosXLabel.Content = value.ToString();
            }
            else
            {
                delObject = value;
                posUpdateDelegate = UpdateX;
                Dispatcher.BeginInvoke(posUpdateDelegate, delObject);
            }
        }

        public void UpdateY(float value)
        {
            if (robotPosYLabel.Dispatcher.CheckAccess())
            {
                robotPosYLabel.Content = value.ToString();
            }
            else
            {
                delObject = value;
                posUpdateDelegate = UpdateY;
                Dispatcher.BeginInvoke(posUpdateDelegate, delObject);
            }
        }

        public void UpdateAngle(float value)
        {
            if (robotAngleLabel.Dispatcher.CheckAccess())
            {
                robotAngleLabel.Content = value.ToString();
            }
            else
            {
                delObject = value;
                posUpdateDelegate = UpdateAngle;
                Dispatcher.BeginInvoke(posUpdateDelegate, delObject);
            }
        }

        public void UpdateEngines(int lEngine, int rEngine)
        {
            if (leftEngineProgressBar.Dispatcher.CheckAccess())
            {
                leftEngineProgressBar.Value = lEngine;
                rightEngineProgressBar.Value = rEngine;
            }
            else
            {
                delObjects[0] = lEngine;
                delObjects[1] = rEngine;
                engUpdateDelegate = UpdateEngines;
                Dispatcher.BeginInvoke(engUpdateDelegate, delObjects);
            }
        }
#endregion
    }
}
