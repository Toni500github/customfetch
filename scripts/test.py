string = input(">> ")

dollarSignIndex = 0

while True:
    oldDollarSignIndex = dollarSignIndex
    dollarSignIndex = string.find('$', dollarSignIndex)
    if dollarSignIndex == -1 or dollarSignIndex <= oldDollarSignIndex:
        print("Nothing to parse!")
        break

    c = string[dollarSignIndex]

    # check for bypass
    if dollarSignIndex > 0 and string[dollarSignIndex - 1] == '\\':
        print("Bypassed!")
        continue

    print("No bypass detected, checking bracket type..")

    match string[dollarSignIndex+1]:
        case '(':
            print("Found an eval command!")
            command = ""
            endBracketIndex = -1

            for i in range(dollarSignIndex+2, len(string)):
                if string[i] == ')' and string[i-1] != '\\':
                    endBracketIndex = i
                    break
                elif string[i] == ')':
                    command = command[:-1]

                command += string[i]

            if endBracketIndex == -1:
                print("ERROR: Opened tag is not closed.")
                exit(1)
            
            string = string.replace(string[dollarSignIndex:endBracketIndex+1], repr(eval(command)))
        case '{':
            print("Found a layout command!")
        case _: # neither
            print("Found nothing after the $, leaving it alone!")

print(string)
