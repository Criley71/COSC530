awk '
BEGIN { RS=""; FS="\n" }  # RS="" treats blank lines as record separators
{
    line1 = $1
    line2 = $2
    print "Comparing:"
    print "OUT line: " line1
    print "TRACE line: " line2

    if (line1 == line2)
        print "Result: EXACT MATCH"
    else if (index(line2, line1))   # line1 is substring of line2
        print "Result: OUT line is in TRACE line"
    else
        print "Result: DIFFERENT"

    print "-----------------"
}' combined.txt
