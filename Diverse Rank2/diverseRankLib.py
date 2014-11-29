def mergingFactor(pattern1,pattern2):
    """return the merging factor at level l"""
    """pattern1 is pattern at level l pattern2 is pattern at level l+1"""

    count1=len(pattern1)*1.0
    count2=len(pattern2)*1.0
    mf=(count1-1.0)/(count2-1.0)
    return mf


def levelFactor(h,l):
    """return the level factor at level l"""

    lf=(2.0*(h-l))/( (h-1)*h)
    return lf


def diverseRank(pattern,height):
    pattern=set(pattern)
    dr=0
    level=height
    while len(pattern)>1:
        newPattern=[]
        level-=1;
        for i in pattern:
            index=i.rfind('+')
            newPattern.append(i[:index])  
        newPattern=set(newPattern)
        dr+=levelFactor(height,level)*mergingFactor(newPattern,pattern)
        pattern=newPattern
    return dr
