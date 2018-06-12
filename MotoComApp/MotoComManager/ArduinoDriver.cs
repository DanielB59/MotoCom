using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	partial class ArduinoDriver {
		string port;
		int baud;
		public SerialPortStream stream; //TODO: remove public when done.
	}

	partial class ArduinoDriver {
		public ArduinoDriver(string port, int baud = -1) {
			this.port = port;
			stream = new SerialPortStream(port);

			if (-1 == baud) {
				stream.GetPortSettings();
				baud = stream.BaudRate;
			}
			else {
				this.baud = baud;
				stream.BaudRate = baud;
			}

			stream.ReadTimeout = 200;
			stream.WriteTimeout = 200;
			open();
		}

		~ArduinoDriver() => Dispose(false);
	}

	partial class ArduinoDriver : IDisposable {
		public void open() {
			if (!stream.IsOpen) stream.Open();
		}

		public void close() {
			if (stream.IsOpen) stream.Close();
		}

		#region IDisposable Support
		private bool disposedValue = false;

		protected virtual void Dispose(bool disposing) {
			if (!disposedValue) {
				if (disposing) {
					stream.Dispose();
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

	partial class ArduinoDriver {
		ConcurrentQueue<Message> readQueue = new ConcurrentQueue<Message>();
		ConcurrentQueue<Message> writeQueue = new ConcurrentQueue<Message>();

		//SpinLock flushable = new SpinLock();	//TODO: is this required??

		public bool synchronize() {
			int attempts = 0;
			byte[] sync = BitConverter.GetBytes(Convert.ToUInt32(0xFF000));
			try {
				do {
					stream.EndWrite(stream.BeginWrite(sync, 0, Message.messageSize, null, null));
					stream.EndRead(stream.BeginRead(sync, 0, Message.messageSize, null, null));
					//stream.Flush();	//TODO: look into, for some reason causes error with sync
				} while (0xFF000 != BitConverter.ToInt32(sync, 0) && attempts < 5);
			}
			catch {
				//TODO: error handling
				return false;
			}
			return (attempts < 5) ? true : false;
		}

		public IAsyncResult read(AsyncCallback callback = null, object state = null) {
			Message readMessage = new Message();
			try {
				callback += (IAsyncResult result) => {
					if (result.IsCompleted)
						readQueue.Enqueue(readMessage);
				};
				return stream.BeginRead(readMessage, callback, state);
			}
			catch {
				//TODO: error handling
				throw new NotImplementedException();
			}
		}

		public IAsyncResult write(AsyncCallback callback = null, object state = null) {
			Message writeMessage = null;
			try {
				if (writeQueue.TryDequeue(out writeMessage))
					return null;
				else
					return stream.BeginWrite(writeMessage, callback, state);
			}
			catch {
				//TODO: error handling
				throw new NotImplementedException();
			}
		}
	}

	static class SerialPortStreamExtentions {
		public static IAsyncResult BeginRead(this SerialPortStream stream, Message message, AsyncCallback callback, object state)
			=> stream.BeginRead(message, 0, Message.messageSize, callback, state);
		public static IAsyncResult BeginWrite(this SerialPortStream stream, Message message, AsyncCallback callback, object state)
			=> stream.BeginWrite(message, 0, Message.messageSize, callback, state);
	}
}
