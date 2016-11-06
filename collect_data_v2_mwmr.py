import os
from time import sleep
import sys


def get_single_test_avg_results(filename):
	# For each file
	srv_msgs=0
	w_sentMsgs = 0
	w_invokedWrites = 0
	w_completedWrites = 0
	r_sentMsgs = 0
	r_invokedReads = 0
	r_completedReads = 0
	r_slowReads = 0
	r_fastReads = 0
	r_avgtime = 0
	
	r_avgCommtime = 0
	r_avgComptime = 0

	#For the MWMR
	w_avgtime = 0
	w_avgCommtime = 0
	w_avgComptime = 0
	numW_completed = 0

	numR_completed = 0


	with open(filename) as f:
		file_lines = []
		for line in f:
			if ("** WRITER_" in line) or (">>>>" in line) or ("** READER_" in line) or ("** SERVER_" in line):
				file_lines.append(line.rstrip())
				if("** SERVER_" in line):
					srv_msgs += int(line.split("=")[1].split(" ")[0])
				if("** WRITER_" in line):
					w_sentMsgs += int(line.split("Msgs=")[1].split(",")[0])
					w_invokedWrites += int(line.split("InvokedWrites=")[1].split(",")[0])
					
					temp_completed = int(line.split("CompletedWrites=")[1].split(",")[0])
					if temp_completed > 0:
						numW_completed += 1
					w_completedWrites += temp_completed
					#w_completedWrites = int(line.split("CompletedWrites=")[1].split(",")[0])
					
					w_aveTotal = line.split("AveOpTime=")[1].split("s")[0]
					if( w_aveTotal != "nan" and w_aveTotal != "-inf" and w_aveTotal != "inf" ):
						w_avgtime += float(line.split("AveOpTime=")[1].split("s")[0])


					w_AveCom = float(line.split("AveCommTime=")[1].split("s")[0])
					if( w_AveCom != "nan" and w_AveCom != "-inf" and w_AveCom != "inf" ):
						w_avgCommtime += float(w_AveCom)

					w_AveComp = float(line.split("AvgCompTime=")[1].split("s")[0])	
					if( w_AveComp != "nan" and w_AveComp != "-inf" and w_AveComp != "inf" ):
						w_avgComptime += float(w_AveComp)

				if("** READER_" in line):
					r_sentMsgs += int(line.split("Msgs=")[1].split(",")[0])
					r_invokedReads += int(line.split("InvokedReads=")[1].split(",")[0])
					
					completed = int(line.split("CompletedReads=")[1].split(",")[0])
					if completed > 0:	
						numR_completed += 1
					r_completedReads += completed 

					if(protocol == "abd") or (protocol == "abd-mwmr") : #ABD and ABD-MWMR
						r_slowReads += int(line.split("4EXCH_reads=")[1].split(",")[0])
						r_fastReads += 0
					elif (protocol == "oh-Sam") or (protocol == "MwImp") or (protocol == "oh-Mam"): #ohSAM, ohMam and Impossibility MWMR
						r_slowReads += int(line.split("3EXCH_reads=")[1].split(",")[0])
						r_fastReads += 0
					elif (protocol == "SwImp"): #Impossibility for SWMR
						r_slowReads += int(line.split("2EXCH_reads=")[1].split(",")[0])
						r_fastReads += 0
					elif (protocol == "oh-Fast") or (protocol == "oh-SamEX") or (protocol == "oh-MamEX"): #Extensions
						r_slowReads += int(line.split("3EXCH_reads=")[1].split(",")[0])
						r_fastReads += int(line.split("2EXCH_reads=")[1].split(",")[0])
					else: #Hybrid #Semifast
						r_slowReads += int(line.split("4EXCH_reads=")[1].split(",")[0])
						r_fastReads += int(line.split("2EXCH_reads=")[1].split(",")[0])
				
					aveTotal = line.split("AveOpTime=")[1].split("s")[0]
					if( aveTotal != "nan" and aveTotal != "-inf" and aveTotal != "inf" ):  	
						r_avgtime += float(line.split("AveOpTime=")[1].split("s")[0])
						## print "** AverageTime = " + aveTotal
					
					aveComm = line.split("AveCommTime=")[1].split("s")[0]
					if( aveComm != "nan" and aveComm != "-inf" and aveComm != "inf"):  
						r_avgCommtime += float(aveComm)		#ok
						## print "** AverageCommunicationTime = " + aveComm
					
					aveComp = line.split("AvgCompTime=")[1].split("s")[0]
					if( aveComp != "nan" and aveComp != "-inf" and aveComp != "inf"):  
						r_avgComptime += float(aveComp)		#ok
						## print "** AverageComputationTime = " + aveComp
					#r_avgComptime += float(line.split("AvgCompTime=")[1].split("s")[0])		#ok
	
	# To be sure not to divide by 0 at any time and break everything
	ratio=0
	percent=0
	if(r_slowReads+r_fastReads!=0):
		ratio=r_fastReads/(r_slowReads+r_fastReads)
		percent=(r_slowReads/(r_fastReads+r_slowReads))*100
	
	#### AVG FOR READERS
	if (numR_completed == 0): 
		r_avgtime = 0
		r_avgCommtime = 0
		r_avgComptime = 0
	else:
		r_avgtime = r_avgtime/numR_completed
		r_avgCommtime = r_avgCommtime/numR_completed
		r_avgComptime = r_avgComptime/numR_completed

	#### AVG FOR WRITERS
	if (numW_completed == 0): 
		w_avgtime = 0
		w_avgCommtime = 0
		w_avgComptime = 0
	else:
		w_avgtime = w_avgtime/numW_completed
		w_avgCommtime = w_avgCommtime/numW_completed
		w_avgComptime = w_avgComptime/numW_completed
	# we have to return this: "fastOps slowOps readRatio readPrcnt readAvgT rdrMsgs wtMsgs srvMsgs\n"
	#return r_fastReads , r_slowReads , ratio , percent , r_avgtime/numR , r_sentMsgs , w_sentMsgs, srv_msgs
	# we have to return this: "fastOps slowOps readRatio readPrcnt readAvgT readAvgCommT readAvgCompT rdrMsgs wtMsgs srvMsgs\n"
	return r_fastReads , r_slowReads , ratio , percent , r_avgtime , r_avgCommtime , r_avgComptime , r_sentMsgs , w_sentMsgs, srv_msgs, w_avgtime , w_avgCommtime , w_avgComptime




def create_output_file_for_scenario(drct,mode):
	directory = drct
		#create directory if needed
	if not os.path.exists(os.path.dirname(directory)):
		os.makedirs(os.path.dirname(directory))

	#open the file for this execution output
	with open(directory, "w") as text_file:
		if (mode==1):
			text_file.write("Protocol Version #Servers #Readers #Writers rIntvl wIntvl fastOps slowOps readRatio readPrcnt readAvgT readCommAvgT readCompAvgT rdrMsgs wrtMsgs srvMsgs writeAvgT writeCommAvgT writeCompAvgT\n")
		else:
			text_file.write("Protocol Version #Servers #Readers #Writers rIntvl wIntvl fastOps slowOps readRatio readPrcnt readAvgT readCommAvgT readCompAvgT rdrAvgMsgs TotRdrAvgMsgs wrtAvgMsgs srvAvgMsgs TotSrvAvgMsgs CmpltTotal AvgReadsPerNode writeAvgT writeCommAvgT writeCompAvgT\n")
	return directory

def execute():
	main_avg_fastOps=0
	main_avg_slowOps=0
	main_avg_readRatio=0
	main_avg_readPrcnt=0
	main_avg_readTime=0
	main_avg_readCommTime=0 #
	main_avg_readCompTime=0 #
	main_avg_rdrMsgs=0
	main_avg_wrtMsgs=0
	main_avg_srvMsgs=0

	main_avg_writeTime=0
	main_avg_writeCommTime=0 #
	main_avg_writeCompTime=0 #

	success_tests=0
	
	seed=0

	# for each test (count for 1 to 6) - 6 not included
	for t in range(1,tests+1):
		print "        executing #test=" + str(t)
		
		scc_directory = "output/logs_for_fails_"+str(fail)+"/servers_equal_"+str(numServers)+"/"+str(alg)+"/readers_writers_"+str(numReaders)+"_"+str(numWriters)+"/version_"+str(Version)+"/read_interval_"+str(rInterval)+"_write_interval_"+str(wInterval)+"_/test_"+str(t)+".txt"
		#if execute flag raised - invoke the command otherwise just parse the output
		if(executeFlag==1 and not os.path.isfile(scc_directory)):
			#create a file for each test
			scc_directory = create_output_file_for_scenario(scc_directory, 1)
			#create a commad and redirection of the output
			command = "./waf --run '"+str(executable)+" --version="+str(version)+" --failures="+str(fail)+" --servers="+str(numServers)+" --readers="+str(numReaders)+" --writers="+str(numWriters)+" --rInterval="+str(rInterval)+" --wInterval="+str(wInterval)+" --seed="+str(seed)+"' >> "+str(scc_directory)+" 2>&1"
			#execute the command
			os.system(command)

		# open the file of the result
		avgs_list = []
		avgs_list = get_single_test_avg_results(scc_directory)
		# process it and store the avg for this file to the avg file for those specific tests.
		# at list we will get this at positions 0,1,2...7 "fastOps slowOps readRatio readPrcnt readAvgT rdrMsgs wrtMsgs srvMsgs\n"
		if(t==1):
			avg_results_directory = create_output_file_for_scenario("output/logs_for_fails_"+str(fail)+"/servers_equal_"+str(numServers)+"/"+str(alg)+"/readers_writers_"+str(numReaders)+"_"+str(numWriters)+"/version_"+str(Version)+"/read_interval_"+str(rInterval)+"_write_interval_"+str(wInterval)+"_/_AVERAGE.txt", 1)
		else:
			avg_results_directory = "output/logs_for_fails_"+str(fail)+"/servers_equal_"+str(numServers)+"/"+str(alg)+"/readers_writers_"+str(numReaders)+"_"+str(numWriters)+"/version_"+str(Version)+"/read_interval_"+str(rInterval)+"_write_interval_"+str(wInterval)+"_/_AVERAGE.txt"
		
		with open(avg_results_directory, "a") as avg_text_file:
			# we have to write this: "Protocol Version #Servers #Readers #Writers rIntvl wIntvl fastOps slowOps readRatio readPrcnt readAvgT readCOmmTime readCompTime rdrMsgs wrtMsgs srvMsgs writeAvgT writeCOmmTime writeCompTime\n"
			output = str(alg) + " " + str(Version)+" "+ str(numServers)+" "+ str(numReaders)+" "+ str(numWriters)+" "+ str(rInterval)+" "+ str(wInterval)+" "+ str(avgs_list[0])+" " + str(avgs_list[1])+" "+ str(avgs_list[2])+" "+ str(avgs_list[3])+" "+ str(avgs_list[4])+" "+ str(avgs_list[5])+" "+ str(avgs_list[6])+" "+ str(avgs_list[7])+" "+ str(avgs_list[8])+" "+ str(avgs_list[9])+ str(avgs_list[10])+" "+ str(avgs_list[11])+" "+ str(avgs_list[12])+"\n"
			avg_text_file.write(output)
		##################
		#Average the values for the main file
		main_avg_fastOps += avgs_list[0]
		main_avg_slowOps += avgs_list[1]
		main_avg_readRatio += avgs_list[2]
		main_avg_readPrcnt += avgs_list[3]
		main_avg_readTime += avgs_list[4]
		main_avg_readCommTime += avgs_list[5]
		main_avg_readCompTime += avgs_list[6]
		main_avg_rdrMsgs += avgs_list[7]
		main_avg_wrtMsgs += avgs_list[8]
		main_avg_srvMsgs += avgs_list[9]
		# New for the MWMR
		main_avg_writeTime += avgs_list[10]
		main_avg_writeCommTime += avgs_list[11]
		main_avg_writeCompTime += avgs_list[12]
		seed += 19

		if avgs_list[4] > 0:
			success_tests += 1
	
	# Once all the tests finish now we have to AVG_ALL the test and write them to the main file!!!
	# Process them here!
	main_avg_fastOps = main_avg_fastOps /success_tests
	main_avg_slowOps = main_avg_slowOps / success_tests
	main_avg_readRatio = main_avg_readRatio / success_tests
	main_avg_readPrcnt = main_avg_readPrcnt / success_tests
	main_avg_readTime = main_avg_readTime / success_tests
	main_avg_readCommTime = main_avg_readCommTime / success_tests
	main_avg_readCompTime = main_avg_readCompTime / success_tests
	main_avg_Total_rdrMsgs = main_avg_rdrMsgs / success_tests
	main_avg_rdrMsgs = main_avg_rdrMsgs / success_tests / numReaders
	main_avg_wrtMsgs = main_avg_wrtMsgs / success_tests / numWriters
	main_avg_Total_srvMsgs = main_avg_srvMsgs / success_tests
	main_avg_srvMsgs = main_avg_srvMsgs / success_tests / numServers
	
	# For the MWMR
	main_avg_writeTime = main_avg_writeTime / success_tests
	main_avg_writeCommTime = main_avg_writeCommTime / success_tests
	main_avg_writeCompTime = main_avg_writeCompTime / success_tests

	# Now write them to the main file
	with open(main_directory, "a") as main_text_file:
		# we have to write this: "Protocol Version #Servers #Readers rIntvl wIntvl fastOps slowOps readRatio readPrcnt readAvgT rdrAvgMsgs wrtAvgMsgs srvAvgMsgs\n"
                output = str(alg) + " " + str(Version)+" "+ str(numServers)+" "+ str(numReaders)+" "+ str(numWriters)+" "+ str(rInterval)+" "+ str(wInterval)+" "+ str(main_avg_fastOps)+" " + str(main_avg_slowOps)+" "+ str(main_avg_readRatio)+" "+ str(main_avg_readPrcnt)+" "+ str(main_avg_readTime)+" "+ str(main_avg_readCommTime)+" "+ str(main_avg_readCompTime)+ " "+ str(main_avg_rdrMsgs)+" "+str(main_avg_Total_rdrMsgs)+" "+ str(main_avg_wrtMsgs)+" "+ str(main_avg_srvMsgs)+" "+str(main_avg_Total_srvMsgs)+" "+str((main_avg_fastOps + main_avg_slowOps))+" "+str(float((main_avg_fastOps + main_avg_slowOps)/numReaders))+str(main_avg_writeTime)+" "+ str(main_avg_writeCommTime)+" "+ str(main_avg_writeCompTime)+"\n"
		main_text_file.write(output)
# WE ARE DONE

#############################################################################
#############################################################################
################################## MAIN #####################################
#############################################################################
#############################################################################


### Set this to false to run the Multiple Writer Algorithms
Single_Writer_Algorithms = False #False #True
executeFlag=1


abd = "abd" 
abd_mwmr = "abd-mwmr"

oh_sam = "oh-Sam"
oh_sam_ex = "oh-SamEX"

oh_mam = "oh-Mam"
oh_mam_ex = "oh-MamEX"

imp_sw = "SwImp"
imp_mw = "MwImp"

#semifast = "Semifast"
#hybridfast = "HybridFast"
#oh_fast = "oh-Fast"

alg =""
executable=""
protocol = ""
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
wrts_start = 1 
wrts_stop=1
wrts_step=1  #No worries we will end up doing 10,20,40,80,100 in the MWMR
vrsn_start=0
vrsn_stop=1
vrsn_step=1
num_tests =0 #(the total number of tests for all executions for SW or MW)
tests_counter=0

if (Single_Writer_Algorithms == True):
	protocols = [ ["abd", "am-abd"], ["oh-Sam", "am-ohSam"], ["oh-SamEX", "am-ohSamEX"],  ["SwImp", "am-SwImp"]]
	num_tests = 1200
else:
	protocols = [ ["abd-mwmr", "am-abd-mwmr"], ["oh-Mam", "am-ohMam"], ["oh-MamEX", "am-ohMamEX"],  ["MwImp", "am-MwImp"]]
	wrts_start = 10 
	wrts_stop=100
	wrts_step=10 #No worries we will end up doing 10,20,40,80,100 in the MWMR
	wInterval_start = 40 #(means 4.0)
	wInterval_stop = 40
	wInterval_step = 10
	rInterval_stop = 46
	num_tests = 6000

topologies = [ "p2p" , "star-p2p" ]
srvrs_start=10 #10
srvrs_stop=30
srvrs_step=5
fail_start=1
fail_stop=1 
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

# if len(sys.argv) == 2:
# 	if (int(sys.argv[1]) == 1):
#             print "Algorithm ABD\n"
#             prtcl_start=1
#             prtcl_stop=1
# 	elif(int(sys.argv[1]) == 2):
#             print "Algorithm oh-Sam\n"
#             prtcl_start=2
#             prtcl_stop=2
# 	elif(int(sys.argv[1]) == 3):
#             print "Algorithm SemiFast\n"
#             prtcl_start=3
#             prtcl_stop=3
# 	elif(int(sys.argv[1]) == 4):
#             print "Algorithm ccHybrid\n"
#             prtcl_start=4
#             prtcl_stop=4
# 	elif(int(sys.argv[1]) == 5):
#             print "Algorithm oh-Fast\n"
#             prtcl_start=5
#             prtcl_stop=5
# 	elif(int(sys.argv[1]) == 6):
#             print "Algorithm ABD SPIKE\n"
#             prtcl_start=6
#             prtcl_stop=6
# 	elif(int(sys.argv[1]) == 7):
#             print "Algorithm oh-Sam SPIKE\n"
#             prtcl_start=7
#             prtcl_stop=7
# 	elif(int(sys.argv[1]) == 8):
#             print "Algorithm oh-Fast SPIKE\n"
#             prtcl_start=8
#             prtcl_stop=8
# 	elif(int(sys.argv[1]) == 9):
#             print "Algorithm ABD STAR\n"
#             prtcl_start=9
#             prtcl_stop=9
# 	elif(int(sys.argv[1]) == 10):
#             print "Algorithm oh-Sam STAR\n"
#             prtcl_start=10
#             prtcl_stop=10
# 	elif(int(sys.argv[1]) == 11):
#             print "Algorithm oh-Fast STAR\n"
#             prtcl_start=11
#             prtcl_stop=11
#         elif(int(sys.argv[1]) == 30):
#             print "Version fixInt"
#             vrsn_start=0
#             vrsn_stop=0
#         elif(int(sys.argv[1]) == 31):
#             print "Version randInt"
#             vrsn_start=1
#             vrsn_stop=1
# 	elif(int(sys.argv[1]) == 50):
#             executeFlag=0
# 	else:
#             print "Not Valid Argument\n"
#             exit(0)



#for all the fail scenario (1,3)
for fail in range(fail_start,fail_stop+1,fail_step):
    for version in range(vrsn_start, vrsn_stop+1, vrsn_step):
        if(version==0):
            Version="fixInt"
        else:
            Version = "randInt"

        print "For Version="+str(Version)+":"
        main_results_dir = "output/ALL_RESULTS_fail_"+str(fail)+"_version_"+str(Version)+".txt"
        print "*** The Main Averaged Results will be at: " + str(main_results_dir)+" ***"
        main_directory = create_output_file_for_scenario(main_results_dir,0)

        for numServers in range(srvrs_start, srvrs_stop+1, srvrs_step):
        	print "--Scenario for Servers="+str(numServers)+"--"
        	for topology in topologies:
        		for protocol, executable in protocols:
        			alg = protocol +"-"+ topology
        			executable = executable +"-"+ topology
        			print "  For Algorithm="+str(executable)+":"

        			for numReaders in range(rdrs_start,rdrs_stop+1, rds_step):
        				if (numReaders==10) or (numReaders==20) or (numReaders==40) or (numReaders==80) or (numReaders==100):
        					print "    For Readers="+str(numReaders)+":"
        					for numWriters in range(wrts_start,wrts_stop+1, wrts_step):
        						if (numWriters==1) or (numWriters==10) or (numWriters==20) or (numWriters==40) or (numWriters==80) or (numWriters==100):
        							print "    For Writers="+str(numWriters)+":"
        							for readInterval in range(rInterval_start, rInterval_stop+1, rInterval_step):
        								rInterval = float(readInterval)/10
        								print "      For readInterval="+str(rInterval)+":"
        								for writeInterval in range(wInterval_start, wInterval_stop+1, wInterval_step):
        									wInterval = float(writeInterval)/10
        									print "       For writeInterval="+str(wInterval)+":"
        									execute()
        									tests_counter = tests_counter + 1
        									if (tests_counter % 100 == 0):
        										print "Completed Tests: ", tests_counter, " out of ", num_tests, " ."


print "\n\nAll the work is done, Script Exiting..."




