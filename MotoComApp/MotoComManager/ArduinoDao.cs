using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	public partial class ArduinoDao : IDisposable {
		private static ArduinoDao instance = null;

		public static ArduinoDao Instance {
			get {
				if (null == instance)
					instance = new ArduinoDao();
				return instance;
			}
		}

		public static EventHandler<RJCP.IO.Ports.SerialDataReceivedEventArgs> readHandler = (s, e) => {
			Console.WriteLine("reading");
			(s as ArduinoDriver).read();
			Message msg = null;
			(s as ArduinoDriver).readQueue.TryDequeue(out msg);
			if (null != msg)
				MainWindow.dispatcher.InvokeAsync(() => Instance.inBoundList.Insert(0, msg));
		};

		private ArduinoDao() {
			//TODO: move to App
			//return;
			Task.Run(() => {
				while (true) {
					try {
						//if (flag) {
						ArduinoDriver[] copy = new ArduinoDriver[viewList.Count];
						Instance.viewList.CopyTo(copy, 0);
						foreach (ArduinoDriver driver in copy) {
							if (null != driver && !driver.isDisposed) {
								if (0 < driver.stream.BytesToRead) {
									driver.read();
								}
								Message msg = null;
								driver.readQueue.TryDequeue(out msg);
								if (null != msg)
									MainWindow.dispatcher.InvokeAsync(() => inBoundList.Insert(0, msg));
							}
						}
						//}
					}
					catch {
					}
				}
			});
		}

		~ArduinoDao() => Dispose(false);

		#region IDisposable Support
		private bool disposedValue = false;

		protected virtual void Dispose(bool disposing) {
			if (!disposedValue) {
				if (disposing) {
					foreach (ArduinoDriver driver in drivers.Values)
						driver.Dispose();
				}

				disposedValue = true;
			}
		}

		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		public bool isDisposed => disposedValue;
		#endregion
	}

	public partial class ArduinoDao {
		public ArduinoDriver selectedDriver = null;
		public Dictionary<string, ArduinoDriver> drivers = new Dictionary<string, ArduinoDriver>();

		public ObservableCollection<ArduinoDriver> viewList = new ObservableCollection<ArduinoDriver>();
		public ObservableCollection<Message> inBoundList = new ObservableCollection<Message>();
		public ObservableCollection<Message> outBoundList = new ObservableCollection<Message>();
	}

	public partial class ArduinoDao {
		public void enqueueMessage(Message msg) {
			selectedDriver.writeQueue.Enqueue(msg);
		}

		public void dequeueMessage(out Message msg) {
			selectedDriver.readQueue.TryDequeue(out msg);
		}

		//volatile bool flag = true;
		public void scanDevices(bool test = false) {
			//flag = false;
			lock (this) {
				if (!test) MainWindow.dispatcher.InvokeAsync(viewList.Clear);
				foreach (PortDescription port in SerialPortStream.GetPortDescriptions()) {
					ArduinoDriver driver = null;
					try {
						if (!drivers.ContainsKey(port.Port)) {
							driver = new ArduinoDriver(port.Port, 9600);
							if (driver.synchronize()) {
								drivers.Add(port.Port, driver);
								if (!test) MainWindow.dispatcher.InvokeAsync(() => viewList.Add(driver));
							}
							else
								driver.Dispose();
						}
						else {
							driver = drivers[port.Port];
							driver.reOpen();
							if (driver.synchronize())
								if (!test) MainWindow.dispatcher.InvokeAsync(() => viewList.Add(driver));
								else {
									drivers.Remove(port.Port);
									driver.Dispose();
								}
						}
					}
					catch {
						if (null != driver)
							driver.Dispose();
					}
				}
				//flag = true;
			}
		}
	}
}
