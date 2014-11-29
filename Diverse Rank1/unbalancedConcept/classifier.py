def classify(rules, data, defVal):
    #print 'Rules: ', rules
    #print 'Data: ', data
    #print '-'*20
    labelledData = []
    #dataItem = data[0]

    for dataItem in data:
        """
        Used for-else construct
        if no if-condition is true then else is used
        """
        for rule in rules:
            if rule[0].issubset(dataItem):
                labelledData.append((dataItem, rule[1]))
                break
        else:
            labelledData.append((dataItem, defVal))

    #print 'LabelledData: \t', labelledData
    return labelledData

def convertRulesToFS(rules):
    result = []
    for rule in rules:
        #print rule
        rule=list(rule)
        rule=sorted(rule,key=lambda x:999 if "_class" in x else 0)
        temp = (frozenset(rule[:-1]), rule[-1])
        result.append(temp)
    return result

if __name__ == '__main__':

    rules = [[ 'A', 'B', 'C', 'P'],
            ['A', 'D', 'Q'],
            ['E', 'R']]

    dataSet = [['A', 'D', 'F'],
            ['A', 'B', 'C', 'D', 'E', 'F'],
            ['A', 'E'],
            ['E', 'F', 'G'],
            ['A', 'B', 'C', 'D'],
            ['A', 'B', 'F']]

    rules = convertRulesToFS(rules)
    dataSet = [frozenset(i) for i in dataSet]

    print "Rules: ", rules
    print "Data: ", dataSet

    classifiedData = classify(rules, dataSet, 'S')
    
    print "Classified Data: ", classifiedData
