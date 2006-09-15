import xml.parsers.expat
import os
import re

currentNode=''
currentAttr={}
currentNodeData=''
addedFiles=[]
modedFiles=[]
movedFiles=[]
allFiles=[]

def start_element(name, attrs):
    global currentNode
    global currentAttr
    global currentNodeData
    global addedFiles
    global modedFiles
    global movedFiles
    currentNode=name
    if name=='patch':
        currentAttr=attrs
        addedFiles=[]
        modedFiles=[]
        movedFiles=[]
    elif name=='removed_lines' or name=='added_lines' or name=='replaced_tokens':
        pass
    elif name=='move':
        movedFiles.append((attrs['from'],attrs['to']))
        allFiles.append(attrs['from'])
        allFiles.append(attrs['to'])
    else:
        currentNodeData=''
    if name=='changelog':
        print """Change log
==========

.. contents::
"""
def end_element(name):
    global currentNodeData
    global allFiles
    def replaceLink(matchobj):
        return " "
    currentNodeData=re.subn(r"\S*_($|\s)",lambda a: '!'+a.group(0),currentNodeData.strip())[0]
    #currentNodeData=re.subn("_ ",replaceLink,currentNodeData.strip())[0]
    #if currentNode=='name' and len(str(currentNodeData).strip()):
    if name=='name' and len(str(currentNodeData).strip()):
        print "\n",currentNodeData
        print '-'*len(currentNodeData),"\n"
        print ":Author:",currentAttr['author']
        print ":Date:",currentAttr['local_date']
        print ":Patch: `%s`__"%currentAttr['hash']
        print "\n__ http://www.darkarts.co.za/repository/vultures/_darcs/patches/%s"%currentAttr['hash']
        print ""
    elif name=='comment':
        print "::\n\n\t%s\n"%currentNodeData.strip().replace("\n","\n\t")
    elif name=='add_file':
        addedFiles.append(currentNodeData.strip())
        allFiles.append(currentNodeData.strip())
    elif name=='modify_file':
        modedFiles.append(currentNodeData.strip())
        allFiles.append(currentNodeData.strip())
    elif name=='patch':
        if len(addedFiles):
            print "Added files:\n"
            for newfile in addedFiles:
                print " - `%s`_"%newfile
            print ""    
        if len(modedFiles):
            print "Modified files:\n"
            for modfile in modedFiles:
                print " - `%s`_"%modfile
            print ""    
        if len(movedFiles):
            print "Renamed/moved files:\n"
            for movefile in movedFiles:
                print " - `%s`_ to `%s`_"%(movefile)
            print ""    
    elif name=='changelog':
        print ""
        for filename in allFiles:
            print ".. _%(fn)s: http://www.darkarts.co.za/repository/vultures/%(fn)s"%{'fn':filename}
def char_data(data):
    global currentNodeData
    currentNodeData+=data

p = xml.parsers.expat.ParserCreate()

p.StartElementHandler = start_element
p.EndElementHandler = end_element
p.CharacterDataHandler = char_data

p.Parse(os.popen('darcs changes --summary --xml-output').read())
