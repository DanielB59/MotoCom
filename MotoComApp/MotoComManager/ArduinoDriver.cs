using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using RJCP.IO;
using RJCP.IO.Ports;
using System.Collections.ObjectModel;

namespace MotoComManager {
	public partial class ArduinoDriver {
		string port;
		int baud;
		public SerialPortStream stream; //TODO: remove public when done.
	}

	public partial class ArduinoDriver {
		public ArduinoDriver(string port, int baud = -1, int ReadTimeout = 500, int WriteTimeout = 200) {
			this.port = port;
			stream = new SerialPortStream(port);

			if (-1 == baud) {
				stream.GetPortSettings();
				this.baud = stream.BaudRate;
			}
			else {
				this.baud = baud;
				stream.BaudRate = baud;
			}

			stream.ReadTimeout = ReadTimeout;
			stream.WriteTimeout = WriteTimeout;
			//stream.Handshake = Handshake.Rts;
			open();
			synchronize();
		}

		~ArduinoDriver() => Dispose(false);
	}

	public partial class ArduinoDriver : IDisposable {
		public void open() {
			if (!stream.IsOpen) stream.Open();
		}

		public void close() {
			if (stream.IsOpen) stream.Close();
		}

		public void reOpen() {
			close();
			open();
		}

		public bool isOpen => stream.IsOpen;

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

		public bool isDisposed => disposedValue;
		#endregion
	}

	public partial class ArduinoDriver {
		public ConcurrentQueue<Message> readQueue = new ConcurrentQueue<Message>();
		public ConcurrentQueue<Message> writeQueue = new ConcurrentQueue<Message>();

		//SpinLock flushable = new SpinLock();	//TODO: is this required??

		const int limit = 5;
		public bool synchronize() {
			int attempts = 0;
			Message sync = null;
			try {
				do {
					sync = new Message(0x7F000);
					writeQueue.Enqueue(sync);
					stream.EndWrite(write());
					stream.Flush();
					sync = null;
					stream.EndRead(read());
					stream.Flush();
					readQueue.TryDequeue(out sync);
				} while (0x7F000 != sync.MessageValue && attempts++ < limit);
			}
			catch {
				//TODO: error handling
				return false;
			}
			return (attempts < limit) ? true : false;
		}

		public IAsyncResult read(AsyncCallback callback = null, object state = null) {
			Message readMessage = new Message();
			
			try {
				if (stream.CanRead) {// && 0 < stream.BytesToRead) {	//TODO: fix, no idea why this isn't working properly...
					callback += (result) => {
						if (result.IsCompleted) {
							readMessage.MessageValue = BitConverter.ToUInt32(readMessage.MessageBytes, 0);
							readQueue.Enqueue(readMessage);
						}
					};
					return stream.BeginRead(out readMessage, callback, state);
				}
				else
					return null;
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
				if (stream.CanWrite)
					if (writeQueue.TryDequeue(out writeMessage))
						return stream.BeginWrite(writeMessage, callback, state);
				
				return null;
			}
			catch (Exception e) {
				//TODO: error handling
				Console.WriteLine(e.StackTrace);
				throw new NotImplementedException();
			}
		}
	}

	public partial class ArduinoDriver {
		public override string ToString() => "[" + stream.PortName + ", " + stream.BaudRate + "]";
	}

	static class SerialPortStreamExtentions {
		public static IAsyncResult BeginRead(this SerialPortStream stream, out Message message, AsyncCallback callback, object state)
			=> stream.BeginRead((message = new Message()).MessageBytes, 0, Message.messageSize, callback, state);
		public static IAsyncResult BeginWrite(this SerialPortStream stream, Message message, AsyncCallback callback, object state)
			=> stream.BeginWrite(message.MessageBytes, 0, Message.messageSize, callback, state);
	}
}
