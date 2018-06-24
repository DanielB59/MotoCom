using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading;
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
using System.Windows.Threading;
using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window {
		public class MotoUnitItem {
			public UInt32 ID { get; private set; }
			public MotoUnitItem(UInt32 id) => ID = id;
			public static implicit operator UInt32(MotoUnitItem item) => item.ID;
			public override string ToString() => "MotoUnit [" + ID + "]";
		}

		ObservableCollection<ArduinoDriver> list = ArduinoDao.Instance.viewList;

		public ObservableCollection<Message> inBoundList = ArduinoDao.Instance.inBoundList;
		public ObservableCollection<Message> outBoundList = ArduinoDao.Instance.outBoundList;
		public ObservableCollection<MotoUnitItem> toList = new ObservableCollection<MotoUnitItem>();

		//temporary solution
		public static MainWindow instance = null;
		public static Dispatcher dispatcher = null;

		public MainWindow() {
			InitializeComponent();

			deviceList.ItemsSource = list;
			inBoundList.CollectionChanged += inboundHandle;
			inboundLog.ItemsSource = inBoundList;
			outboundLog.ItemsSource = outBoundList;

			clusterDevices.ItemsSource = toBox.ItemsSource = toList;
			toBox.SelectedIndex = 0;
			castTypeBox.ItemsSource = Enum.GetValues(typeof(Message.BroadcastType));
			castTypeBox.SelectedIndex = 0;
			dataBox.ItemsSource = Enum.GetValues(typeof(Message.MessageData));
			dataBox.SelectedIndex = 0;

			instance = this;
			dispatcher = Dispatcher;
		}

		private bool isSync = false;
		private void syncButton_Click(object sender, RoutedEventArgs e) {
			if (!isSync) {
				isSync = true;
				deviceList.SelectedItem = ArduinoDao.Instance.selectedDriver = null;
				Task.Run(() => ArduinoDao.Instance.scanDevices()).ContinueWith((task) => isSync = false);
			}
		}

		private void sendButton_Click(object sender, RoutedEventArgs e) {
			Task.Run(() => {
				if (null != ArduinoDao.Instance.selectedDriver)
					Dispatcher.InvokeAsync(() => {
						Message msg = new Message();
						msg[Message.Field.from] = 0x0;  //TODO: fix?
						msg[Message.Field.to] = (null != toBox.SelectedItem)?(UInt32)(MotoUnitItem)toBox.SelectedItem:0x0;  //TODO: fix
						msg[Message.Field.broadcastType] = (UInt32)(Message.BroadcastType)castTypeBox.SelectedItem;
						msg[Message.Field.senderType] = (UInt32)Message.SenderType.hq;
						msg[Message.Field.messageData] = (UInt32)(Message.MessageData)dataBox.SelectedItem;
						messageSend(msg);
					});
			});
		}

		private void messageSend(Message msg) {
			ArduinoDao.Instance.enqueueMessage(msg);
			ArduinoDao.Instance.selectedDriver.write();
			Dispatcher.InvokeAsync(() => outBoundList.Insert(0, msg));
		}

		private static UInt32 counter = 0;
		private static UInt32 Counter { get => ++counter % 0x40; }
		public void inboundHandle(object sender, NotifyCollectionChangedEventArgs e) {
			Task.Run(() => {
				switch (e.Action) {
					case NotifyCollectionChangedAction.Add:
						Message msg = e.NewItems[0] as Message;
						switch (msg[Message.Field.messageData]) {
							case (UInt32)Message.MessageData.requestID:
								messageSend(new Message(0x0, Counter, Message.BroadcastType.single, Message.SenderType.hq, Message.MessageData.assignID));
								Dispatcher.InvokeAsync(() => toList.Add(new MotoUnitItem(counter)));
								break;
							default:
								Console.WriteLine("a message was recieved!");
								break;
						}
						break;
				}
			});
		}

		private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e) {
			ArduinoDao.Instance.Dispose();
		}

		private void deviceList_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			ArduinoDriver driver = ArduinoDao.Instance.selectedDriver = deviceList.SelectedItem as ArduinoDriver;
			if (null != driver)
				selectLabel.Content = driver.ToString();
			else
				selectLabel.Content = "<selected>";
		}

		private void simuButton_Click(object sender, RoutedEventArgs e) {
			Task.Run(() => {
				if (null != ArduinoDao.Instance.selectedDriver)
					Dispatcher.InvokeAsync(() => {
						Message msg = new Message();
						msg[Message.Field.from] = 0;  //TODO: fix?
						msg[Message.Field.to] = 0;  //TODO: fix
						msg[Message.Field.broadcastType] = (UInt32)(Message.BroadcastType)castTypeBox.SelectedItem;
						msg[Message.Field.senderType] = (UInt32)Message.SenderType.hq;  //TODO: ok?
						msg[Message.Field.messageData] = (UInt32)(Message.MessageData)dataBox.SelectedItem;
						inBoundList.Add(msg);
					});
			});
		}
	}
}
