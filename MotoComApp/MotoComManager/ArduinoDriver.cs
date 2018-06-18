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
			//stream.ReadBufferSize = sizeof(UInt32);
			//stream.WriteBufferSize = sizeof(UInt32);
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
			int attempts = 1, retrieved = 0, retryCount = 0;
			byte[] sync = null;
			try {
				do {
					sync = BitConverter.GetBytes(Convert.ToUInt32(0xFF000));
					//stream.EndWrite(stream.BeginWrite(sync, 0, Message.messageSize, null, null));
					Console.WriteLine("before {0:x}", BitConverter.ToUInt32(sync, 0));
					stream.Write(sync, 0, Message.messageSize);
					stream.Flush();
					sync = BitConverter.GetBytes(Convert.ToUInt32(0x0));
					Console.WriteLine("clean {0:x}", BitConverter.ToUInt32(sync, 0));
					//stream.EndRead(stream.BeginRead(sync, 0, Message.messageSize, null, null));
					stream.Read(sync, 0, Message.messageSize);
					//while (retrieved < Message.messageSize && retryCount++ < 100)
					//sync[retrieved++] = stream.ReadByte();
					//retrieved += stream.Read(sync, retrieved, Message.messageSize - retrieved);

					Console.WriteLine("after {0:x}", BitConverter.ToUInt32(sync, 0));// & 0x7FFF);
					stream.Flush();
				} while (0xFF000 != BitConverter.ToInt32(sync, 0) && attempts++ < 100);
			}
			catch {
				//TODO: error handling
				return false;
			}
			return (attempts < 3) ? true : false;
		}

		public IAsyncResult read(AsyncCallback callback = null, object state = null) {
			Message readMessage = new Message();
			try {
				if (stream.CanRead && 0 < stream.BytesToRead) {
					Console.WriteLine("in");
					callback += (IAsyncResult result) => {  //TODO: modify/enhance
						if (result.IsCompleted) {
							readMessage.MessageValue = BitConverter.ToUInt32(readMessage.MessageBytes, 0);
							readQueue.Enqueue(readMessage);
							Console.WriteLine("enqueued");
						}
					};

					return stream.BeginRead(out readMessage, callback, state);
					/*callback(null);
					return res;*/
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

	static class SerialPortStreamExtentions {
		public static IAsyncResult BeginRead(this SerialPortStream stream, out Message message, AsyncCallback callback, object state)
			=> stream.BeginRead((message = new Message()).MessageBytes, 0, Message.messageSize, callback, state);
		public static IAsyncResult BeginWrite(this SerialPortStream stream, Message message, AsyncCallback callback, object state)
			=> stream.BeginWrite(message, 0, Message.messageSize, callback, state);
	}
}
