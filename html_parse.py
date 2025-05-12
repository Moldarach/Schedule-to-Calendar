import sys
import os
import re
from pathlib import Path
import uuid
import shutil

# navigate to https://sdb.admin.uw.edu/sisStudents/uwnetid/schedule.aspx and 
# use that html page as input. unfortunately quarter start and end dates 
# are currently hard-coded, can probably change that later by having this
# refer to academic calendar and look at local date or something


# "3/30/2025", for some reason all the events will display on the starting date 
# on google calendar and i have absolutely no idea why it does that
QUARTER_START = "20250330" 

# "6/7/2025", ensure events show up on 6th as the end date seems to be exclusive
QUARTER_END =  "20250607" 

# class containing all data for calendar event
class Event:
    def __init__(self, start_time: str, end_time: str, name: str, 
                 location: str, days: list[str], ):
        
        self.start_time = start_time
        self.end_time = end_time
        self.name = name
        self.location = location
        self.days = days
    
# takes in file path and if the file exists and is an html file,
# it will read out the html into a string
def read_html_file(file_name: str) -> str:
    # Check if file has .html extension
    if not file_name.lower().endswith('.html'):
        print("Error: File is not an HTML file.")
        return None

    # Check if the file exists
    if not os.path.isfile(file_name):
        print("Error: File does not exist.")
        return None

    # Read the file contents into a string
    try:
        with open(file_name, 'r', encoding='utf-8') as file:
            html_content = file.read()
            return html_content
    except Exception as e:
        print(f"Error reading file: {e}")
        return None

# perform regex on the entire html string to pick out the useful pieces
# for building each calendar event
def regex(html_string: str) -> None:
    pattern = r'<td class=[\"|\']mono nowrap[\"|\']>[A-Z&nbsp;]+ [0-9]+.*</td>\n.*\n.*\n.*\n\t<td class=[\"|\']mono nowrap[\"|\']>[MWThF]*</td>\n.*\n.*'
    res = re.findall(pattern, html_string)
    
    for course in res:
        # split html by newline to extract needed information
        split = course.splitlines()

        # extract class name
        start = 24
        end = 24
        while (split[0][end] != '<'):
            end+=1
        name = split[0][start:end]
        name = name.replace("&nbsp;", " ")

        # extract days
        start = 25
        end = 25
        while (split[4][end] != '<'):
            end+=1
        days = split[4][start:end]
        # need to convert these days into a list format for easier ICS file creation
        day_list = []
        for i in range(len(days)):
            if (days[i] == 'T' and i < len(days)-1 and days[i+1] == 'h'):
                day_list.append("TH")
            elif (days[i] == 'T'):
                day_list.append("TU")
            elif (days[i] == 'M'):
                day_list.append("MO")
            elif (days[i] == 'W'):
                day_list.append("WE")
            elif (days[i] == 'F'):
                day_list.append("FR")
        
        # extract times
        start = 25
        end = 25
        while (split[5][end] != '<'):
            end+=1
        times = split[5][start:end]
        # strip out non-breaking space, replace with zero for easier integer parsing
        times = times.replace("&nbsp;", "0")
        # need to convert times to military time
        replace_start = times[:4]
        replace_end = times[-4:]

        # NOTE this does not work for PMP classes that run from 6pm to 9pm
        if (int(times[0:2]) < 8):
            replace_start = str(int(times[0:2]) + 12) + times[2:4]
        if (int(times[5:7]) < 8):
            replace_end = str(int(times[-4:-2]) + 12) + times[-2:]

        # extract location
        location_pattern = r'>[A-Z0-9 ]+</a> [A-Z0-9]+<'
        location = ""
        curr = re.findall(location_pattern, split[6])
        if (len(curr) == 0):
            location = "TBA"
        else:
            start = 1
            end = 1
            while (curr[0][end] != '<'):
                end+=1
            location = curr[0][start:end] + " "
            start = end + 5
            end = start
            while (curr[0][end] != '<'):
                end+=1
            location += curr[0][start:end]

        event = Event(replace_start, replace_end, name, location, day_list)
        create_ics(event)
    

# create actual ICS file given the event details
def create_ics(event: Event) -> None:
    # this code is terrible and seems to use 2 different ways to check
    # if a file exists (compared to in read_html) as well as write to a file

    file_path = Path(event.name + ".ics")

    if file_path.exists():
        print(f"{file_path} already exists. Not overwriting.")
        return
    else:
        # copy starter ics data (which should be the same for all events)
        try:
          shutil.copyfile("starter_ics.txt", file_path)
        except Exception as e:
            print(f"shutil.copyfile error occured {e}")
            return
        
        uid = uuid.uuid4()
        # DTSTART;TZID=America/Los_Angeles:20250331T093000
        remaining_content = "DTSTART;TZID=America/Los_Angeles:" + QUARTER_START + "T" \
          + event.start_time + "00\n"
        # DTEND;TZID=America/Los_Angeles:20250331T103000
        remaining_content += "DTEND;TZID=America/Los_Angeles:" + QUARTER_START + "T" \
          + event.end_time + "00\n"
        
        day_str = ""
        for index, day in enumerate(event.days):
            day_str = day_str + day 
            if (index != len(event.days)-1):
                day_str = day_str + ","
        
        # RRULE:FREQ=WEEKLY;WKST=SU;UNTIL=20250607T065959Z;BYDAY=MO,TU,WE,FR
        remaining_content += "RRULE:FREQ=WEEKLY;WKST=SU;UNTIL=" + QUARTER_END + "T" \
          "065959Z;BYDAY=" + day_str + "\n"
        
        # DTSTAMP:20250506T183209Z
        remaining_content += "DTSTAMP:20250506T183209Z\n"

        # UID:insert_some_uid_here
        remaining_content += "UID:" + str(uid) + "\n"

        # LOCATION:THO 125
        remaining_content += "LOCATION:" + event.location + "\n"

        # STATUS:CONFIRMED
        remaining_content += "STATUE:CONFIRMED\n"

        # SUMMARY:EE 331 A
        remaining_content += "SUMMARY:" + event.name + "\n"

        # remainder
        remaining_content += "TRANSP:OPAQUE\nEND:VEVENT\nEND:VCALENDAR"

        #file_path.write_text(remaining_content)
        with file_path.open("a") as f:
            f.write(remaining_content)

    '''
DTSTART;TZID=America/Los_Angeles:20250331T093000
DTEND;TZID=America/Los_Angeles:20250331T103000
RRULE:FREQ=WEEKLY;WKST=SU;UNTIL=20250607T065959Z;BYDAY=MO,TU,WE,FR
DTSTAMP:20250506T183209Z
UID:insert_some_uid_here
LOCATION:THO 125
STATUS:CONFIRMED
SUMMARY:EE 331 A
TRANSP:OPAQUE
END:VEVENT
END:VCALENDAR
    '''

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python html_parse.py <filename.html>")
        sys.exit(1)

    filename = sys.argv[1]
    html_string = read_html_file(filename)
    
    if html_string is not None:
        print("HTML content successfully read.")
        regex(html_string)
