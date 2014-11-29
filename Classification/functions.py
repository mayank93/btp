def ruleMiner(freqPatterns, classLabels):
    rules=[]
    for pattern in freqPatterns:
        for label in classLabels:
            #rules.append(set(pattern+[label]))
            rules.append(pattern + (label,))
    return rules

def ranker(rules, diverseFunc, confidenceFunc, diverseWt = 0.50, confidenceWt = 0.50, cutoff = 0.2):
    def scoreFunc(rule, df = diverseFunc, cf = confidenceFunc, dw = diverseWt, cw = confidenceWt):
        score1 = df(rule)
        score2 = cf(rule)
        return score1 * dw + score2 * cw
    #return sorted(rules, key = scoreFunc)
    return [i for i in sorted(rules, key = scoreFunc) if scoreFunc(i) >= cutoff]


def retX(x,y):
    return x,y

def highorder(func,*arg):
    def loworder(x):
        return func(x,*arg)
    return loworder

def confidence(rule,transactions,classLabels):
    """rule(A->B) is whose confidence need to be calculated,rule is a set of item where the item that belong to label class is B and rest is A """
    """transaction is a list of set of items"""
    """labelClass is a list of all label which are to be predicted"""
    rule=set(rule)
    for i in classLabels:
        if i in rule:
            B=set([i])
            break
    A=rule-B
    Acount=0
    ABcount=0
    for i in transactions:
#        print i
#        print A
#        print B
        if A.issubset(i):
            Acount+=1
        if rule.issubset(i):
            ABcount+=1
    if Acount==0:
        return 0;
    return (ABcount*1.0)/Acount


if __name__=="__main__":
    ruleArray = [3, 5, 12, 6, 1, 2, 9]
#    ranker(ruleArray, retX, retX)
    print highorder(retX,2)(1)
