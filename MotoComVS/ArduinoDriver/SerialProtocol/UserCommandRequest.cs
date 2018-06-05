namespace ArduinoDriver.SerialProtocol {
	public class UserCommandRequest : ArduinoRequest {
		public UserCommandRequest(byte cmdValue)
			: base(CommandConstants.UserCmd) {
			Bytes.Add(cmdValue);
		}
	}
}
