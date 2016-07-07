import os
from time import sleep
import sys


def get_single_test_avg_results(filename,numR):
	# For each file
	srv_msgs=0;
	w_sentMsgs = 0
	w_invokedWrites = 0
	w_completedWrites = 0
	w_avgtime = 0
	r_sentMsgs = 0
	r_invokedReads = 0
	r_completedReads = 0
	r_slowReads = 0
	r_fastReads = 0
	r_avgtime = 0


	with open(filename) as f:
		file_lines = []
		for line in f:
			if ("** WRITER_" in line) or (">>>>" in line) or ("** READER_" in line) or ("** SERVER_" in line):
				file_lines.append(line.rstrip())
				if("** SERVER_" in line):
					srv_msgs += int(line.split("=")[1].split(" ")[0])
				if("** WRITER_" in line):
					w_sentMsgs = int(line.split("Msgs=")[1].split(",")[0])
					w_invokedWrites = int(line.split("InvokedWrites=")[1].split(",")[0])
					w_completedWrites = int(line.split("CompletedWrites=")[1].split(",")[0])
					w_avgtime = float(line.split("AveOpTime=")[1].split("s")[0])
				if("** READER_" in line):
					r_sentMsgs += int(line.split("Msgs=")[1].split(",")[0])
					r_invokedReads += int(line.split("InvokedReads=")[1].split(",")[0])
					r_completedReads += int(line.split("CompletedReads=")[1].split(",")[0])
					if(protocol==1): #ABD
						r_slowReads += int(line.split("4EXCH_reads=")[1].split(",")[0])
						r_fastReads += 0
					elif (protocol==2): #ohSAM, oh-fast
						r_slowReads += int(line.split("3EXCH_reads=")[1].split(",")[0])
						r_fastReads += 0
					elif (protocol==5): #oh-fast
						r_slowReads += int(line.split("3EXCH_reads=")[1].split(",")[0])
						r_fastReads += int(line.split("2EXCH_reads=")[1].split(",")[0])
					else: #Hybrid #Semifast
						r_slowReads += int(line.split("4EXCH_reads=")[1].split(",")[0])
						r_fastReads += int(line.split("2EXCH_reads=")[1].split(",")[0])
					r_avgtime += float(line.split("AveOpTime=")[1].split("s")[0])
	
	# To be sure not to divide by 0 at any time and break everything
	ratio=0;
	percent=0;
	if(r_slowReads+r_fastReads!=0):
		ratio=r_fastReads/(r_slowReads+r_fastReads)
		percent=(r_slowReads/(r_fastReads+r_slowReads))*100
	
	# we have to return this: "fastOps slowOps readRatio readPrcnt readAvgT rdrMsgs wtMsgs srvMsgs\n"
	return r_fastReads , r_slowReads , ratio , percent , r_avgtime/numR , r_sentMsgs , w_sentMsgs, srv_msgs



def create_output_file_for_scenario(drct,mode):
	#directory = "output/ALL_RESULTS.fail."+str(fail)+"servers."+str(numServers)+".txt"
	directory = drct
		#create directory if needed
	if not os.path.exists(os.path.dirname(directory)):
		os.makedirs(os.path.dirname(directory))

	#open the file for this execution output
	with open(directory, "w") as text_file:
		if (mode==1):
			text_file.write("Protocol Version #Servers #Readers rIntvl wIntvl fastOps slowOps readRatio readPrcnt readAvgT rdrMsgs wrtMsgs srvMsgs\n")
		else:
			text_file.write("Protocol Version #Servers #Readers rIntvl wIntvl fastOps slowOps readRatio readPrcnt readAvgT rdrAvgMsgs TotRdrAvgMsgs wrtAvgMsgs srvAvgMsgs TotSrvAvgMsgs CmpltTotal AvgReadsPerNode\n")
	return directory

def execute():
	main_avg_fastOps=0
	main_avg_slowOps=0
	main_avg_readRatio=0
	main_avg_readPrcnt=0
	main_avg_readTime=0
	main_avg_rdrMsgs=0
	main_avg_wrtMsgs=0
	main_avg_srvMsgs=0
	
	seed=0

	# for each test (count for 1 to 6) - 6 not included
	for t in range(1,tests+1):
		#those variables will be used to write back to the main file
		###print "        executing #test=" + str(t)
		

		#create a file for each test
		scc_directory = create_output_file_for_scenario("output/logs_for_fails_"+str(fail)+"/servers_equal_"+str(numServers)+"/"+str(alg)+"/readers_"+str(numReaders)+"/version_"+str(Version)+"/read_interval_"+str(rInterval)+"_write_interval_"+str(wInterval)+"_/test_"+str(t)+".txt", 1)
		#create a commad and redirection of the output
		command = "./waf --run '"+str(executable)+" --version="+str(version)+" --failures="+str(fail)+" --servers="+str(numServers)+" --readers="+str(numReaders)+" --rInterval="+str(rInterval)+" --wInterval="+str(wInterval)+" --seed="+str(seed)+"' >> "+str(scc_directory)+" 2>&1"
		#execute the command
		os.system(command)
		# open the file of the result
		avgs_list = []
		avgs_list = get_single_test_avg_results(scc_directory,numReaders)
		# process it and store the avg for this file to the avg file for those specific tests.
		# at list we will get this at positions 0,1,2...7 "fastOps slowOps readRatio readPrcnt readAvgT rdrMsgs wrtMsgs srvMsgs\n"
		if(t==1):
			avg_results_directory = create_output_file_for_scenario("output/logs_for_fails_"+str(fail)+"/servers_equal_"+str(numServers)+"/"+str(alg)+"/readers_"+str(numReaders)+"/version_"+str(Version)+"/read_interval_"+str(rInterval)+"_write_interval_"+str(wInterval)+"_/_AVERAGE.txt", 1)
		else:
			avg_results_directory = "output/logs_for_fails_"+str(fail)+"/servers_equal_"+str(numServers)+"/"+str(alg)+"/readers_"+str(numReaders)+"/version_"+str(Version)+"/read_interval_"+str(rInterval)+"_write_interval_"+str(wInterval)+"_/_AVERAGE.txt"
		
		with open(avg_results_directory, "a") as avg_text_file:
			# we have to write this: "Protocol Version #Servers #Readers rIntvl wIntvl fastOps slowOps readRatio readPrcnt readAvgT rdrMsgs wrtMsgs srvMsgs\n"
			output = str(alg) + " " + str(Version)+" "+ str(numServers)+" "+ str(numReaders)+" "+ str(rInterval)+" "+ str(wInterval)+" "+ str(avgs_list[0])+" " + str(avgs_list[1])+" "+ str(avgs_list[2])+" "+ str(avgs_list[3])+" "+ str(avgs_list[4])+" "+ str(avgs_list[5])+" "+ str(avgs_list[6])+" "+ str(avgs_list[7])+"\n"
			avg_text_file.write(output)
		##################
		#Average the values for the main file
		main_avg_fastOps += avgs_list[0]
		main_avg_slowOps += avgs_list[1]
		main_avg_readRatio += avgs_list[2]
		main_avg_readPrcnt += avgs_list[3]
		main_avg_readTime += avgs_list[4]
		main_avg_rdrMsgs += avgs_list[5]
		main_avg_wrtMsgs += avgs_list[6]
		main_avg_srvMsgs += avgs_list[7]
		seed += 19
	
	# Once all the tests finish now we have to AVG_ALL the test and write them to the main file!!!
	# Process them here!
	main_avg_fastOps = main_avg_fastOps / tests
	main_avg_slowOps = main_avg_slowOps / tests
	main_avg_readRatio = main_avg_readRatio / tests
	main_avg_readPrcnt = main_avg_readPrcnt / tests
	main_avg_readTime = main_avg_readTime / tests
	main_avg_Total_rdrMsgs = main_avg_rdrMsgs / tests
	main_avg_rdrMsgs = main_avg_rdrMsgs / tests / numReaders
	main_avg_wrtMsgs = main_avg_wrtMsgs / tests
	main_avg_Total_srvMsgs = main_avg_srvMsgs / tests
	main_avg_srvMsgs = main_avg_srvMsgs / tests / numServers
	


	# Now write them to the main file
	with open(main_directory, "a") as main_text_file:
		# we have to write this: "Protocol Version #Servers #Readers rIntvl wIntvl fastOps slowOps readRatio readPrcnt readAvgT rdrAvgMsgs wrtAvgMsgs srvAvgMsgs\n"
		output = str(alg) + " " + str(Version)+" "+ str(numServers)+" "+ str(numReaders)+" "+ str(rInterval)+" "+ str(wInterval)+" "+ str(main_avg_fastOps)+" " + str(main_avg_slowOps)+" "+ str(main_avg_readRatio)+" "+ str(main_avg_readPrcnt)+" "+ str(main_avg_readTime)+" "+ str(main_avg_rdrMsgs)+" "+str(main_avg_Total_rdrMsgs)+" "+ str(main_avg_wrtMsgs)+" "+ str(main_avg_srvMsgs)+" "+str(main_avg_Total_srvMsgs)+" "+str((main_avg_fastOps + main_avg_slowOps))+" "+str(float((main_avg_fastOps + main_avg_slowOps)/numReaders))+"\n"
		main_text_file.write(output)
# WE ARE DONE

abd = "abd" 
oh_sam = "oh-Sam"
semifast = "Semifast"
hybridfast = "HybridFast"
oh_fast = "oh-Fast"
alg =""
executable=""
tests = 5
Version="fixInt"# "randInt"

#############################################################################
#############################################################################
###### NOTE FOR THE FINAL UNCOMMENT THIS ####################################
#############################################################################
#############################################################################
# _start is the initial
# _stop is the last one - included!
wInterval_start = 40 #(means 4)
wInterval_stop = 40
wInterval_step = 10
rInterval_start = 23 #(means 2.3)
rInterval_stop = 69
rInterval_step = 23
rdrs_start = 10 
rdrs_stop=100
rds_step=10  #No worries we will end up doing 10,20,40,80,100
vrsn_start=1
vrsn_stop=2
vrsn_step=1
prtcl_start = 1
prtcl_stop = 5
prtcl_step=1
srvrs_start=10
srvrs_stop=30
srvrs_step=5
fail_start=1
fail_stop=2
fail_step=1
bar=0
#############################################################################
#############################################################################
############# FOR THE MOMENT USE THE BELLOW #################################
#############################################################################
#############################################################################
# wInterval_start = 40	
# wInterval_stop = 40
# wInterval_step = 10 
# rInterval_start = 23 #(means 2.3)
# rInterval_stop = 46  #(means 4.6)
# rInterval_step = 23  #(means 2.3)
# rdrs_start = 10      #number of readers to start
# rdrs_stop=100 		 #number of readers to stop (all will be 100)
# rds_step=10  		 #No worries we will end up doing 10,20,40,80,100 if stop = 100
# vrsn_start=0         #All versions = 2 (0-fixInt, 1-randInt)
# vrsn_stop=1          #for the moment do only the fixInt
# vrsn_step=1
# prtcl_start = 1      #All protocols = 5, ABD, ohSam, Semifast, Hybrid, ohFast 
# prtcl_stop = 5       #For the moment do only ABD. Use vale 2,3,4,5 to do also OhSam and others..
# prtcl_step=1
# srvrs_start=10       #Number of Servers
# srvrs_stop=30        #Number of Servers in the final should be 30
# srvrs_step=5
# fail_start=1         #Number of failures. For the moment do only one. 
# fail_stop=1
# fail_step=1
################################################################################

print "Starting Script...\n"

if len(sys.argv) == 2:
	if (int(sys.argv[1]) == 1):
		print "Algorithm ABD\n"
		prtcl_start=1
		prtcl_stop=1
	elif(int(sys.argv[1]) == 2):
		print "Algorithm oh-Sam\n"
		prtcl_start=2
		prtcl_stop=2
	elif(int(sys.argv[1]) == 3):
		print "Algorithm SemiFast\n"
		prtcl_start=3
		prtcl_stop=3
	elif(int(sys.argv[1]) == 4):
		print "Algorithm ccHybrid\n"
		prtcl_start=4
		prtcl_stop=4
	elif(int(sys.argv[1]) == 5):
		print "Algorithm oh-Fast\n"
		prtcl_start=5
		prtcl_stop=5
	else:
		print "Wrong Algorithm Selection\n"
		exit(0)



#for all the fail scenario (1,3)
for fail in range(fail_start,fail_stop+1,fail_step):
	#print fail
	#range(start, stop, step) (last step is not included
	main_results_dir = "output/ALL_RESULTS_fail_"+str(fail)+".txt"
	###print "*** For Failures="+str(fail)+" the Main Averaged Results will be at: " + str(main_results_dir)+" ***"
	main_directory = create_output_file_for_scenario(main_results_dir,0)
	for numServers in range(srvrs_start, srvrs_stop+1, srvrs_step):
		###print "--Scenario for Servers="+str(numServers)+"--"
		# for each protocol (up to 6)
		for protocol in range(prtcl_start,prtcl_stop+1,prtcl_step):
			# for each number of readers (up to 10,20,40,80)
			if (protocol==1):
				alg=abd
				executable="am-abd"
			elif (protocol==2):
				alg=oh_sam
				executable="am-ohSam"
			elif (protocol==3):
				alg=semifast;
				executable="am-semifast"
			elif (protocol==4):
				alg=hybridfast;
				executable="am-cchybrid"
			else:
				alg=oh_fast;
				executable="am-ohfast"

			###print "  For Algorithm="+str(executable)+":"
			
			for numReaders in range(rdrs_start,rdrs_stop+1, rds_step):
				if (numReaders==10) or (numReaders==20) or (numReaders==40) or (numReaders==80) or (numReaders==100):
					###print "    For Readers="+str(numReaders)+":"
					for version in range(vrsn_start, vrsn_stop+1, vrsn_step):
						if(version==1):
							Version="fixInt"
						else:
							Version = "randInt"
						bar=bar+1
						sys.stdout.write('\r')
						# the exact output you're looking for:
						sys.stdout.write("[%-100s] %d%%" % ('='*bar, bar))
						sys.stdout.flush()
    					###print "     For Version="+str(Version)+":"
						for readInterval in range(rInterval_start, rInterval_stop+1, rInterval_step):
							rInterval = float(readInterval)/10
							###print "      For readInterval="+str(rInterval)+":"
							for writeInterval in range(wInterval_start, wInterval_stop+1, wInterval_step):
								wInterval = float(writeInterval)/10
								###print "       For writeInterval="+str(wInterval)+":"
								execute()


print "\n\nAll the work is done, Script Exiting..."




