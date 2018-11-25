using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;


namespace MemMap
{

    public class AC
    {
	    static uint migration_num = 0;
        static ulong[] prev_core_stall_cycles;
        static double[] prev_interference;
        static ulong[] prev_memtime;
        static ulong diff_read_cost;
        static ulong diff_write_cost;
        static double u_total = 0;
        static double prev_u_total = 0;
        static bool eff_thresh_inc = false;
        static double Interval;
        static double eff_thresh = 0;
        static double alpha = 0.1;
        static double[] ThreadWeight; 
        static Random randm = new Random();

	public static void decision() {
        	prev_u_total = u_total;	

                u_total = 0;

                /*for (int i = 0; i < Config.N; i++)
                {
                    u_total += (double) (Measurement.core_stall_cycles[i] - prev_core_stall_cycles[i]);
                }

                for (int i = 0; i < Config.N; i++)
                {
                      if (Measurement.memtime[i] != prev_memtime[i])
                      {
                          ThreadWeight[i] = 1.0 - (Measurement.interference[i] - prev_interference[i]) / ((double) (Measurement.memtime[i] - prev_memtime[i])) * ((double) (Measurement.core_stall_cycles[i] - prev_core_stall_cycles[i])) / Interval;
                      }
                      else
                          ThreadWeight[i] = 1.0;
                }

                for (int i = 0; i < Config.N; i++)
                {
                    prev_core_stall_cycles[i] = Measurement.core_stall_cycles[i];
                    prev_interference[i] = Measurement.interference[i];
                    prev_memtime[i] = Measurement.memtime[i];
                }


                double unit = RowStat.t_rd_miss_diff / 27.0;

                if (u_total < prev_u_total)
                {
                      if (eff_thresh_inc)
                      {
                           eff_thresh = eff_thresh + unit;
                      }
                      else
                      {
                           if (eff_thresh >= unit)
                               eff_thresh = eff_thresh - unit;
                      }
                }
                else
                {
                      if (eff_thresh_inc)
                      {
                          if (eff_thresh >= unit)
                                eff_thresh = eff_thresh - unit;
                          else
                                eff_thresh_inc = !eff_thresh_inc;
                      }
                      else
                      {
                          eff_thresh = eff_thresh + unit;
                      }
                      eff_thresh_inc  = !eff_thresh_inc;
                }*/

                migration_num = 0;
	}
  	

	public static void tick()
        {
               //Console.WriteLine("AC policy tick() called\n");
               RowStat.AccessInfo temp;
               ulong rowkey;

               rowkey = RowStat.KeyGen(Row_Migration_Policies.target_req);
 
               if (!RowStat.NVMDict.ContainsKey(rowkey))
                 return;
               
               temp = RowStat.NVMDict[rowkey];

               if (temp.addlist)
                 return;
                    
               uint c_value = temp.counter;
               double c_n = Math.Pow(2, c_value);
               double c_n1 = Math.Pow(2, c_value+1);

               int rand_num = randm.Next(0, 99);
               double prob_val = 100/(c_n1-c_n);

               if(rand_num < prob_val) {
                   temp.counter++;
               }

               c_value = temp.counter;
              //double utility = (temp.ReadMiss * diff_read_cost * temp.ReadMLPnum + temp.WriteMiss * diff_write_cost * temp.WriteMLPnum)  * ThreadWeight[temp.pid];


               if ((c_value >= 7) && (!Migration.migrationlist.Contains(rowkey)))
               {
                    Migration.migrationlist.Add(rowkey);
                    Migration.migrationlistPID.Add(temp.pid);
                    migration_num++;
                    temp.addlist = true;
                    temp.counter = 0;
                    RowStat.NVMDict[rowkey] = temp;
               }
                
         }

	public static void initialize()
	{
                /*DDR3DRAM ddr3_temp1 = new DDR3DRAM (Config.mem.ddr3_type, Config.mem.clock_factor, 0, 0);   
                DDR3DRAM ddr3_temp2 = new DDR3DRAM (Config.mem2.ddr3_type, Config.mem2.clock_factor,0,0);   
                diff_read_cost = ddr3_temp1.timing.tRCD - ddr3_temp2.timing.tRCD;
                diff_write_cost = ddr3_temp1.timing.tWR - ddr3_temp2.timing.tWR;*/
                Interval = Row_Migration_Policies.Interval;
                prev_core_stall_cycles = new ulong[Config.N];
                prev_interference = new double[Config.N];
                prev_memtime = new ulong[Config.N];

                for (int i=0; i<Config.N; i++)
                {
                    prev_core_stall_cycles[i] = 0;
                    prev_interference[i] = 0;
                    prev_memtime[i] = 0;
                }
                ThreadWeight = new double [Config.N];
                for (int i = 0; i < Config.N; i++)
                    ThreadWeight[i] = 1;
	}

	public static void assignE0()
	{
             /*   u_total = 0;

                for (int i = 0; i < Config.N; i++)
                {
                    u_total += (double) (Measurement.core_stall_cycles[i] - prev_core_stall_cycles[i]);
                }
	        
              
                for (int i = 0; i < Config.N; i++)
                {
                      if (Measurement.memtime[i] != prev_memtime[i])
                          ThreadWeight[i] = 1.0 - (Measurement.interference[i] - prev_interference[i]) / ((double) (Measurement.memtime[i] - prev_memtime[i])) * ((double) (Measurement.core_stall_cycles[i] - prev_core_stall_cycles[i])) / Interval;
                      else
                          ThreadWeight[i] = 1.0;
              }

                for (int i = 0; i < Config.N; i++)
                {
                    prev_core_stall_cycles[i] = Measurement.core_stall_cycles[i];
                    prev_interference[i] = Measurement.interference[i];
                    prev_memtime[i] = Measurement.memtime[i];
                }

                double Eff_max = 1;
		foreach (ulong rowkey in RowStat.NVMLookUp)
		{
                	RowStat.AccessInfo temp = RowStat.NVMDict[rowkey];
                        double temp_utility = (temp.ReadMiss * diff_read_cost * temp.ReadMLPnum + temp.WriteMiss * diff_write_cost * temp.WriteMLPnum) * ThreadWeight[temp.pid];	       
                        if (temp_utility > Eff_max)
                               Eff_max = temp_utility;
                }
                eff_thresh = alpha * Eff_max;

    */
	} 

    }
}
