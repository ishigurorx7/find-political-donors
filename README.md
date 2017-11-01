# find-political-donors
Cording Challenge from InsightDataScience
==============================================================================================
                README file

Date:        Tue Oct 31 21:57:44 PDT 2017
Author:     Hisakazu Ishiguro
Email:      hisakazuishiguro@gmail.com
Objective:  README file for 'find political donors' project
==============================================================================================

1. Introduction
================

This is a README file that attached with program named 'find_political_donors'.
The program can be run by running bash script file 'run.sh' which is installed on the
home directory of the package.
The program will read FEC provided election campaign contribution data from specified input data,
and calculate their median value based on ID and zip-code, and write to the output file.
The program was developed on Ubuntu Linux (running on VirtualBox) env using 100% C programming language.
It does not use any special library.

2. Coverage of project.
=======================

This released package support following feature, and there are still missing features:

1) Covered 'median value by zip code' feature.
2) Not covered 'median value by date with sort' feature.
3) Not covered test command to verifying the program.

Note that the program was tested with several different input files,
and it's verified that it generate correct output format without any failures.
The program correctly skip the line if it 'OTHER_ID' field has no empty data.
It also program calculate with rightly adjusted length of zip-code (first 5 digits).
Their output order is matched with the order of input data.

3. How to run the program?
==========================

The program can be run via bash script file named 'run.sh'.
That file is located on the home directory of this package.
Here is the command to run:

% ./run.sh

Above operation will launching following commands:

% make -f Makefile
% ./find_political_donors -f ./input/itcont.txt -o ./output/medianval_by_zip.txt -m 0

As you can see, first script will build executable file if it's required by running 'make'
command. After that it runs 'find_political_donors' command.
The command assumes that their input file located on ./input directory and their name is
'itcont.txt' where it was downloaded from FEC website. The output file will be created
in ./output directory and their name is medianval_by_zip.txt.
Regarding their command argument such as '-f' shown above, see following section
for more detail.

4. 'find_political_donors' command argument
===========================================

There are several command arguments available for 'find_political_donors' command
so that you can run with different input file manually.

The 'find_political_donors' command is created on home directory (same dire of
run.sh). You can check thier avaialble command argument by running with '-h' option:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
% ./find_political_donors -h

Start './find_political_donors' program ...
./find_political_donors usage:[-hvs] -f input_fname -o output_fname

Arguments:
    -v         : Enable verbose mode
    -s         : Do sort before output to file
    -f <fname> : Specify input  file name [Mandentory]
    -o <fname> : Specify output file name [Mandentory]
    -m <num>   : 0[Default]: MEDIAN_BY_ZIP, 1: MEDIAN_BY_DATE
    -h         : Print command usage
%
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Here is the example to run with different input/output files with verbosity enable:

% ./find_political_donors -f ./my_input_file.txt -o ~/home/my_output_file.txt -v -m

The output file is automatically created if it doesn't exist.
'-s' (sort option) and 'm 1' (MEDIAN_BY_DATA) are not implemented yet.

5. Source files and their location
Program was edited on Emacs and compiled using gcc command.

8. How to implement missing features?
====================================

'median val by date' can be implemented using same way.
For 'sort' feature, it need to used different data structure
such as balanced tree instead of linked list.

