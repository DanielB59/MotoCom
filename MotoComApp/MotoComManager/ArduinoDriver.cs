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
		public ConcurrentQueue<Message> readQueue = new ConcurrentQueue<Message>();
		public ConcurrentQueue<Message> writeQueue = new ConcurrentQueue<Message>();

		//SpinLock flushable = new SpinLock();	//TODO: is this required??

		public bool synchronize() {
			int attempts = 1;
			byte[] sync = BitConverter.GetBytes(Convert.ToUInt32(0xFF000));
			try {
				do {
					stream.EndWrite(stream.BeginWrite(sync, 0, Message.messageSize, null, null));
					//sync = BitConverter.GetBytes(Convert.ToUInt32(0x0));
					//for (int i = 0; i < sync.Length; ++i) sync[i] = 0;
					//Thread.Sleep(stream.WriteTimeout*2);
					stream.Flush();
					stream.EndRead(stream.BeginRead(sync, 0, Message.messageSize, null, null));
					Console.WriteLine("{0:x}", BitConverter.ToUInt32(sync, 0));
					stream.Flush();
				} while (0xFF000 != BitConverter.ToInt32(sync, 0) && attempts++ < 3);
			}
			catch {
				//TODO: error handling
				return false;
			}
			return (attempts < 3) ? true : false;
		}

		public int read(AsyncCallback callback = null, object state = null) {
			Message readMessage = new Message();
			try {
				callback += (IAsyncResult result) => {
					//if (result.IsCompleted)
						readQueue.Enqueue(readMessage);
				};
				return stream.BeginRead(out readMessage, callback, state);
			}
			catch (Exception e) {
				//TODO: error handling
				Console.WriteLine(e.StackTrace);
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
		public static int BeginRead(this SerialPortStream stream, out Message message, AsyncCallback callback, object state) {
			byte[] bytes = new byte[Message.messageSize];
			//callback += (res) => message = new Message(BitConverter.ToUInt32(bytes, 0));
			message = null;
			//callback += (IAsyncResult res) => Console.WriteLine(res.IsCompleted);
			//return stream.BeginRead(bytes, 0, Message.messageSize, callback, state);
			stream.Read(bytes, 0, Message.messageSize);
			callback(null);
			return 0;
		}
		public static IAsyncResult BeginWrite(this SerialPortStream stream, Message message, AsyncCallback callback, object state)
			=> stream.BeginWrite(message, 0, Message.messageSize, callback, state);
	}
}
