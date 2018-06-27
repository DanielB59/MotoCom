using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Reflection.Emit;
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
		public abstract class MotoItem {
			public UInt32 ID { get; private set; }
			public MotoItem(UInt32 id) => ID = id;
			public static implicit operator UInt32(MotoItem item) => item.ID;
		}

		public class MotoUnitItem : MotoItem {
			private UInt32 counter = 1;
			public UInt32 Counter { get => (counter) == 0x40 ? counter = 1 : counter++; }
			public MotoUnitItem(UInt32 id): base(id) { }
			public override string ToString() => "MotoUnit [" + ID + "]";
		}

		public class MotoClusterItem : MotoItem {
			public MotoUnitItem idHandler = new MotoUnitItem(0);
			private static UInt32 counter = 1;
			public static UInt32 Counter { get => (counter) == 0x10 ? counter = 1 : counter++; }
			public SolidColorBrush Brush { get; private set; }
			public MotoClusterItem(UInt32 id) : base(id) {
				Random r = new Random();
				switch (r.Next(3)) {
					case 0:
						Brush = new SolidColorBrush(Color.FromArgb(0xFF, (byte)r.Next(0xFF), (byte)r.Next(0xFF), 0x00));
						break;
					case 1:
						Brush = new SolidColorBrush(Color.FromArgb(0xFF, (byte)r.Next(0xFF), 0x00, (byte)r.Next(0xFF)));
						break;
					case 2:
						Brush = new SolidColorBrush(Color.FromArgb(0xFF, 0x00, (byte)r.Next(0xFF), (byte)r.Next(0xFF)));
						break;
				}
			}
			public override string ToString() => "MotoCluster [" + ID + "]";
		}

		ObservableCollection<ArduinoDriver> list = ArduinoDao.Instance.viewList;

		public ObservableCollection<Message> inBoundList = ArduinoDao.Instance.inBoundList;
		public ObservableCollection<Message> outBoundList = ArduinoDao.Instance.outBoundList;
		public ObservableCollection<MotoClusterItem> clusterList = new ObservableCollection<MotoClusterItem>();
		public Dictionary<UInt32, ObservableCollection<MotoItem>> clusterDeviceList = new Dictionary<UInt32, ObservableCollection<MotoItem>>();

		//temporary solution
		public static MainWindow instance = null;
		public static Dispatcher dispatcher = null;

		public MainWindow() {
			InitializeComponent();

			deviceList.ItemsSource = list;
			inBoundList.CollectionChanged += inboundHandle;
			inboundLog.ItemsSource = inBoundList;
			outboundLog.ItemsSource = outBoundList;

			castTypeBox.ItemsSource = Enum.GetValues(typeof(Message.BroadcastType));
			castTypeBox.SelectedIndex = 0;
			dataBox.ItemsSource = new Message.MessageData[] { Message.MessageData.fire, Message.MessageData.advance, Message.MessageData.retreat, Message.MessageData.stopFire, Message.MessageData.bind };
			dataBox.SelectedIndex = 0;
			clusterBox.ItemsSource = clusterList;

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
				Message msg = new Message();
				Dispatcher.InvokeAsync(() => {
					if (null != deviceList.SelectedItem && null != clusterBox.SelectedItem) {
						msg[Message.Field.from] = 0x0;
						msg[Message.Field.to] = (null != clusterDevices.SelectedItem && Message.BroadcastType.all != (Message.BroadcastType)castTypeBox.SelectedItem) ? (UInt32)(MotoItem)clusterDevices.SelectedItem : 0x0;
						msg[Message.Field.broadcastType] = (UInt32)(Message.BroadcastType)castTypeBox.SelectedItem;
						msg[Message.Field.senderType] = (UInt32)Message.SenderType.hq;
						msg[Message.Field.messageData] = (UInt32)(Message.MessageData)dataBox.SelectedItem;
						msg[Message.Field.clusterID] = (null != clusterBox.SelectedItem) ? (UInt32)(MotoItem)clusterBox.SelectedItem : 0x0;
						if (0 < clusterList.Count) {
							msg.BrushBack = (from cluster in clusterList
											 where cluster.ID == msg[Message.Field.clusterID]
											 select cluster.Brush).FirstOrDefault();
							msg.BrushFore = new SolidColorBrush(Color.FromArgb(0xFF, 0xFF, 0xFF, 0xFF));
						}
						messageSend(msg);
					}
				});
			});
		}

		private void messageSend(Message msg) {
			if (outBoundHandle(msg)) {
				ArduinoDao.Instance.enqueueMessage(msg);
				ArduinoDao.Instance.selectedDriver.write();
				Dispatcher.InvokeAsync(() => outBoundList.Insert(0, msg));
			}
		}

		public bool outBoundHandle(Message msg) {
			switch (msg[Message.Field.messageData]) {
				case (UInt32)Message.MessageData.bind:
					if (requestedBind) {
						MotoUnitItem newUnit = new MotoUnitItem(((MotoClusterItem)clusterBox.SelectedItem).idHandler.Counter);
						msg[Message.Field.to] = newUnit.ID;
						msg[Message.Field.broadcastType] = (UInt32)Message.BroadcastType.all;
						msg[Message.Field.clusterID] = (MotoItem)clusterBox.SelectedItem;
						clusterDeviceList[(MotoItem)clusterBox.SelectedItem].Add(newUnit);
						requestedBind = false;
					}
					else
						return false;
					break;
				default:
					Console.WriteLine("a message was sent!");
					break;
			}

			return true;
		}

		private bool requestedBind = false;
		public void inboundHandle(object sender, NotifyCollectionChangedEventArgs e) {
			Task.Run(() => {
				switch (e.Action) {
					case NotifyCollectionChangedAction.Add:
						Message msg = e.NewItems[0] as Message;
						switch (msg[Message.Field.messageData]) {
							case (UInt32)Message.MessageData.requestBind:
								requestedBind = true;
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
				selectHQ.Content = driver.ToString();
			else
				selectHQ.Content = "<selected>";
		}

		private void createButton_Click(object sender, RoutedEventArgs e) {
			MotoClusterItem newCluster = new MotoClusterItem(MotoClusterItem.Counter);
			clusterDeviceList.Add(newCluster.ID, new ObservableCollection<MotoItem>());
			clusterList.Add(newCluster);
			clusterBox.SelectedIndex = clusterList.Count - 1;
			selectUnit.Content = "<selected>";
		}

		private void clusterDevices_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			if (null != clusterDevices.SelectedItem)
				selectUnit.Content = clusterDevices.SelectedItem.ToString();
			else
				selectUnit.Content = "<selected>";
		}

		private void clusterBox_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			clusterDevices.ItemsSource = clusterDeviceList[(MotoItem)clusterBox.SelectedItem];
		}
	}
}
