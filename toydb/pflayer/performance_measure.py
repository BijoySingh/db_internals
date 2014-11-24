log_file = open("raid_log.txt","r")

commands_read = 0
valid_steps = 0
reads = 0
writes = 0
backups = 0

for line in log_file:
	line = line.strip()
	if(line == "STEP" and commands_read != 0):
		valid_steps += 1
		commands_read = 0
	if(line == "STEP"):
		commands_read = 0
	else:
		commands_read += 1
		line = line.split(",")
		if(line[0] == "R01"):
			if(line[1] == "W"):
				writes += 1
			if(line[1] == "R"):
				reads += 1
		else:
			backups += 1

print("Reads : \t\t\t" + str(reads))
print("Writes : \t\t\t" + str(writes))
print("Backups : \t\t\t" + str(backups))
print("Time Steps : \t\t\t" + str(valid_steps))
print("Time Taken By Simple System : \t" + str(writes + reads + backups))
print("Time Taken By RAID 01 System : \t" + str(valid_steps))


