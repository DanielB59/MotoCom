using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using ArduinoDriver;

namespace MotoManager {
	class Program {
		static void Main(string[] args) {
			ArduinoDAO dao = ArduinoDAO.Instance;
			dao.test();
		}
	}
}
