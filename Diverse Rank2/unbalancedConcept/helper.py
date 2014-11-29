def confidence(rule,transaction,labelClass):
    """rule(A->B) is whose confidence need to be calculated,rule is a set of item where the item that belong to label class is B and rest is A """
    """transaction is a list of set of items"""
    """labelClass is a list of all label which are to be predicted"""

    for i in labelClass:
        if i in rule:
            B=set([i])
            break
    A=rule-B
    Acount=0
    ABcount=0
    for i in transaction:
        if A.issubset(i):
            Acount+=1
        if rule.issubset(i):
            ABcount+=1
    if Acount==0:
        return 0;
    return (ABcount*1.0)/Acount
    
if __name__=="__main__":
    rule=set([1,2,3])
    labelClass=[3,4,5]
    trans=[
            set([1,2,3]),
            set([1,2,3]),
            set([1,2,4]),
            set([1,2,3]),
            set([1,2,5]),
            set([1,2,3]),
            set([1,2,3]),
            set([1,2,3]),
            set([1,7,3]),
            set([1,7,3]),
            set([1,2,3])
            ]
    print confidence(rule,trans,labelClass)
