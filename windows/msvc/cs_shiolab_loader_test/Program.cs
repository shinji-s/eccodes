using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace cs_shiolab_loader_test
{
    class Program
    {
        [System.Runtime.InteropServices.DllImport("shiolab_loaders.dll",
            CharSet = System.Runtime.InteropServices.CharSet.Ansi)]
        public static extern Int32 bufr_load(string fname, StringBuilder sb, Int32 capacity);
        static void Main(string[] args)
        {
            Environment.SetEnvironmentVariable(
                "ECCODES_DEFINITION_PATH",
                "../../../../definitions");
            const string input_fname = "../../Z__C_RJTD_20190619000000_OBS_AMDS_Rjp_N1_bufr4.bin";
            const string output_fname = "../../amedas.json";
            var sb = new StringBuilder(65536 * 16 * 5);
            int r = bufr_load(input_fname, sb, sb.Capacity);
            if (0 < r) {
                sb = new StringBuilder(r);
                r = bufr_load(input_fname,sb, sb.Capacity);
                Debug.Assert(r == 0);
            }
            if (r==0)
            {
                System.IO.File.WriteAllText(output_fname, sb.ToString());
            } else
            {
                Console.WriteLine($"buf_load_json() returned {r}.");
            }
            // System.IO.File.Delete(output_fname);
            Console.WriteLine("Goodbye.");
        }
    }
}
