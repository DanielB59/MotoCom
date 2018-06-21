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
				baud = stream.BaudRate;
			}
			else {
				this.baud = baud;
				stream.BaudRate = baud;
			}

			stream.ReadTimeout = ReadTimeout;
			stream.WriteTimeout = WriteTimeout;
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

	public partial class ArduinoDriver {
		public ConcurrentQueue<Message> readQueue = new ConcurrentQueue<Message>();
		public ConcurrentQueue<Message> writeQueue = new ConcurrentQueue<Message>();

		//SpinLock flushable = new SpinLock();	//TODO: is this required??

		public bool synchronize() {
			int attempts = 1, retrieved = 0, retryCount = 0;
			//byte[] sync = null;
			Message sync = null;
			try {
				do {
					sync = new Message(0x7F000);
					byte[] sync2 = BitConverter.GetBytes(0x7F000);
					//stream.EndWrite(stream.BeginWrite(sync, 0, Message.messageSize, null, null));
					Console.WriteLine("before {0}", sync);
					//stream.Write(sync, 0, Message.messageSize);
					//stream.EndWrite(stream.BeginWrite(sync, null, null));
					/*writeQueue.Enqueue(sync);
					stream.EndWrite(write());*/
					stream.EndWrite(stream.BeginWrite(sync2, 0, sizeof(UInt32), null, null));
					//sync = BitConverter.GetBytes(Convert.ToUInt32(0x0));
					sync = null;
					Console.WriteLine("clean {0}", sync);
					//stream.EndRead(stream.BeginRead(sync, 0, Message.messageSize, null, null));
					//stream.Read(sync, 0, Message.messageSize);
					//stream.BeginRead(out sync, null, null);
					stream.EndRead(read());
					Console.WriteLine(readQueue.IsEmpty);
					readQueue.TryDequeue(out sync);
					Console.WriteLine(sync);
					Console.WriteLine("----------------------");
					//while (retrieved < Message.messageSize && retryCount++ < 100)
					//sync[retrieved++] = stream.ReadByte();
					//retrieved += stream.Read(sync, retrieved, Message.messageSize - retrieved);

					//Console.WriteLine("after {0:x}, {1}", BitConverter.ToUInt32(sync, 0), 0xFF000 != BitConverter.ToInt32(sync, 0) && attempts < 10);// & 0x7FFF);
					Console.WriteLine("after {0} {1}", sync, sync.MessageValue.Equals(0x7F000));
					if (sync.MessageValue.Equals(0x7F000)) Console.WriteLine("derp...");
					stream.Flush();
				} while (0x7F000 != sync.MessageValue && attempts++ < 10);
			}
			catch {
				//TODO: error handling
				return false;
			}
			if (attempts < 10) Console.WriteLine("Horray!!");
			return (attempts < 10) ? true : false;
		}

		public IAsyncResult read(AsyncCallback callback = null, object state = null) {
			Message readMessage = new Message();
			Console.WriteLine("----------------------");
			try {
				Console.WriteLine("{0}, {1}", stream.CanRead, 0 < stream.BytesToRead);
				if (stream.CanRead && 0 < stream.BytesToRead) {
					Console.WriteLine("in");
					callback += (IAsyncResult result) => {  //TODO: modify/enhance
						if (result.IsCompleted) {
							readMessage.MessageValue = BitConverter.ToUInt32(readMessage.MessageBytes, 0);
							readQueue.Enqueue(readMessage);
							Console.WriteLine("enqueued");
						}
					};
					Console.WriteLine("+++++++++++++++++++");
					return stream.BeginRead(out readMessage, callback, state);
					/*callback(null);
					return res;*/
				}
				else {
					Console.WriteLine("hi");
					return null;
				}
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
				callback += (res) => Console.WriteLine("sent");
				if (stream.CanWrite) {
					if (writeQueue.TryDequeue(out writeMessage)) {
						return stream.BeginWrite(writeMessage, callback, state);
					}
					else
						return null;
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
