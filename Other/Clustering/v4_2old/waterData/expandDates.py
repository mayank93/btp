import datetime
import sys

std = lambda x: datetime.datetime.strptime(x, 'D-%d/%m/%y')
def remake(x):
    x = std(x)
    return datetime.datetime.strftime(x, "D-%d/%m/%y")

def expandDate(date):
    date = [i.strip('. t\n') for i in date.split('to')]
    date = [i for i in date if len(i) > 0]
    #print date[0]
    #print std(date[0])
    #print date[0], date[1]
    start = std(date[0])
    end = std(date[1])
    day_count = (end - start).days + 1
    retList = []
    for i in (start + datetime.timedelta(n) for n in range(day_count)):
        #print datetime.datetime.strftime("D-%d/%m/%y", i)
        retList.append(datetime.datetime.strftime(i, "D-%d/%m/%y"))
        #x = datetime.datetime.strftime(i, "%e/%m/%y")
        #x = x.strip(' ')
        #x = 'D-' + x
        #retList.append(x)
        #print datetime.datetime.strftime(i.timetuple(), "D-%d/%m/%y")
    return retList

def flattenDates(l):
    #print l
    l = l.strip('. \t\n')
    l = l.split(',')
    #print l
    l = [i.strip('. \t\n') for i in l]
    l = [i for i in l if i is not '']
    res = []
    for i in l:
        if 'to' in i:
            res += expandDate(i)
        else:
            #print i
            res.append(remake(i))
    #print res
    return res

def getDates():
    f = open('dateClass', 'r')
    #l = f.readline()
    #res = flattenDates(l)
    d = {}
    for j, i in enumerate(f):
        #print j
        res = flattenDates(i)
        for k in res:
            d[k] = j + 1
    return d

def makeClusters(d):
    f = open('water-treatment.data.txt', 'r')
    #13 nat clus
    clusters = {}
    for i in range(13):
        clusters[i] = []


    for count, i in enumerate(f):
        r = i.split(',')
        r = [j.strip(' \r\t\n') for j in r]
        r = [j for j in r if len(j) > 0]
        if(len(r) == 0): continue
        try:
            r = remake(r[0])
        except IndexError:
            #print r, x, len(x)
            pass
        try:
            v =  d[r]
            #print v, count + 1
            clusters[v-1].append(count + 1)
        except KeyError:
            print "[[ ERROR ]]", r
            pass

    for count, i in enumerate(clusters.keys()):
        #print "Size: ", len(clusters[i])
        #print i, clusters[i]
        #jklhkhkkhprint i, ":", 
        print ' '.join([str(aa) for aa in clusters[i]])

if __name__ == '__main__':
    d = getDates()
    makeClusters(d)
    pass
