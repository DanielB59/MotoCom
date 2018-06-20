using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using RJCP.IO.Ports;

namespace MotoComManager {
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window {
		private delegate void op();
		op scanTask = ArduinoDao.Instance.scanDevices;
		ObservableCollection<ArduinoDriver> list = ArduinoDao.Instance.viewList;

		public MainWindow() {
			InitializeComponent();

			//temporary solution
			deviceList.ItemsSource = list;
			toBox.ItemsSource = null;
			castTypeBox.ItemsSource = Enum.GetValues(typeof(Message.BroadcastType));
			dataBox.ItemsSource = Enum.GetValues(typeof(Message.MessageData));
		}

		private void scanButton_Click(object sender, RoutedEventArgs e) {
			ArduinoDao.Instance.scanDevices();
		}

		private void sendButton_Click(object sender, RoutedEventArgs e) {
			Message msg = new Message();
			msg[Message.Field.from] = 0;  //TODO: fix
			msg[Message.Field.to] = 0;//(UInt32)(Message.BroadcastType)toBox.SelectedItem;
			msg[Message.Field.broadcastType] = (UInt32)castTypeBox.SelectedItem;
			msg[Message.Field.senderType] = 0;  //TODO: fix
			msg[Message.Field.messageData] = (UInt32)dataBox.SelectedItem;
			ArduinoDao.Instance.enqueueMessage(msg);
		}
	}
}
