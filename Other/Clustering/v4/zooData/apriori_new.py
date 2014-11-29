import os

def runApriori_bin(data_frozenset, minSup):
    #print data_frozenset[:10]
    print "-------------start--------------"
    binInput = 'binInput'
    binOutput = 'binOutput'
    f = open(binInput, 'w+')
    for i in data_frozenset:
        f.write(' '.join(i) + '\n')
    f.close()

    os.system("./apriori_bin -s%0.1f %s %s" % (minSup * 100, binInput, binOutput))

    f = open(binOutput, 'r')

    items = []

    for i in f:
	print i
        i = i.strip(' \t\n').split(' ')
        support = float(i[-1].strip('()'))/100
        i = i[:-1]
        i = [j for j in i if len(j) > 0]
        items.append((tuple(i), support))

    #print items
    #items = runApriori(data_frozenset, minSup)
    #print items
    #print type(items[0][0])
    print items
    print "-------------over--------------"
    return items
