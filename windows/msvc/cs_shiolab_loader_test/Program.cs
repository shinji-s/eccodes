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
        public static extern long bufr_load_amedas(string fname, StringBuilder sb, Int32 capacity);
        static void Main(string[] args)
        {
            const string input_fname = "c:/Users/shinj/Downloads/Z__C_RJTD_20190619000000_OBS_AMDS_Rjp_N1_bufr4.bin";
            const string output_fname = @"C:\Users\Shinj\Documents\src\amedas.json";
            var sb = new StringBuilder(65536 * 16 * 5);
            long r = bufr_load_amedas(input_fname, sb, sb.Capacity);
            if (0 < r) {
                sb = new StringBuilder((int)r);
                r = bufr_load_amedas(input_fname,sb, sb.Capacity);
                Debug.Assert(r == 0);
            }
            // System.IO.File.Delete(output_fname);
            System.IO.File.WriteAllText(output_fname, sb.ToString());
        }
    }
}
