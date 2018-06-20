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
	partial class ArduinoDao : IDisposable {
		private static ArduinoDao instance = null;

		public static ArduinoDao Instance {
			get {
				if (null == instance)
					instance = new ArduinoDao();
				return instance;
			}
		}

		private ArduinoDao() { }

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
		#endregion
	}

	partial class ArduinoDao {
		ArduinoDriver selectedDriver = null;
		string selectedPort = null;
		public Dictionary<string, ArduinoDriver> drivers = new Dictionary<string, ArduinoDriver>();
		public ObservableCollection<ArduinoDriver> viewList = new ObservableCollection<ArduinoDriver>();
	}

	partial class ArduinoDao {
		public void enqueueMessage(Message msg) {
			selectedDriver.writeQueue.Enqueue(msg);
		}

		public void dequeueMessage(out Message msg) {
			selectedDriver.readQueue.TryDequeue(out msg);
		}

		public void scanDevices() {
			ArduinoDriver driver = null;
			//Func<bool> sync = null;
			//Task<bool> sync = null;
			viewList.Clear();
			foreach (PortDescription port in SerialPortStream.GetPortDescriptions()) {
				try {
					if (!drivers.ContainsKey(port.Port)) {
						driver = new ArduinoDriver(port.Port);
						drivers.Add(port.Port, driver);
						viewList.Add(driver);
					}
					else
						try {
							Console.WriteLine("before");
							driver = drivers[port.Port];
							driver.synchronize();
							viewList.Add(driver);
							Console.WriteLine("after");
						}
						catch {
							drivers.Remove(port.Port);
							throw;
						}
						
				}
				catch {
					if (null != driver)
						driver.Dispose();
				}
			}
		}
	}
}
