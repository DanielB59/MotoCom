using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Collections;

namespace MotoComManager {
	public class Message {
		public enum Field : UInt32 { from = 0x3F, to = 0xFC0, broadcastType = 0x3000, senderType = 0xC000, messageData = 0x3F0000, clusterID = 0x3C00000 };

		public enum BroadcastType : UInt32 { all = 0, single = 1, control = 2, distress = 3 };
		public enum SenderType : UInt32 { soldier = 0, commander = 1, hq = 2, external = 3 };
		public enum MessageData : UInt32 { fire = 0, stopFire = 1, advance = 2, retreat = 3, ack = 4, requestID = 5, assignID = 6, isConnected = 7, verifyConnect = 8 };

		public const int messageSize = sizeof(UInt32);

		private UInt32 messageValue;
		public UInt32 MessageValue {
			get => messageValue;

			set {
				value &= 0x3FFFFFF;
				messageValue = value;
				BitConverter.GetBytes(value).CopyTo(MessageBytes, 0);
			}
		}
		public byte[] MessageBytes { get; set; }

		public Message(UInt32 value = 0) {
			MessageBytes = new byte[messageSize];
			MessageValue = value;
		}

		public Message(UInt32 from, UInt32 to, BroadcastType castType, SenderType senderType, MessageData data, UInt32 clusterID) : this() {
			this[Field.from] = from;
			this[Field.to] = to;
			this[Field.broadcastType] = (UInt32)castType;
			this[Field.senderType] = (UInt32)senderType;
			this[Field.messageData] = (UInt32)data;
			this[Field.clusterID] = clusterID;
		}

		public static implicit operator byte[] (Message message) {
			return message.MessageBytes;
		}

		private int getOffset(Field field) {
			switch (field) {
				case Field.from: return 0;
				case Field.to: return 6;
				case Field.broadcastType: return 12;
				case Field.senderType: return 14;
				case Field.messageData: return 16;
				case Field.clusterID: return 22;
				default: return -1;
			}
		}

		public UInt32 this[Field field] {
			get => (MessageValue & (UInt32)field) >> getOffset(field);

			set {
				UInt32 reminder = MessageValue & (~(UInt32)field);
				value = value << getOffset(field);
				MessageValue = reminder | (value & (UInt32)field);
			}
		}

		public override string ToString() {
			StringBuilder builder = new StringBuilder();
			builder.Append("[from: " + this[Field.from]);
			builder.Append(", to: " + this[Field.to]);
			builder.Append(", broadcastType: " + this[Field.broadcastType]);
			builder.Append(", senderType: " + this[Field.senderType]);
			builder.Append(", Data: " + this[Field.messageData]);
			builder.Append(", clusterID: " + this[Field.clusterID]);
			builder.Append(" | value: " + MessageValue + "]");
			return builder.ToString();
		}
	}
}
