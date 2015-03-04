/*
  thorfinn thorfinn@binf.ku.dk 13march 2012
  not threadsafe
*/

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

#include "faidx.h"
#include "refFinder.h"

struct cmp_str
{
   bool operator()(char const *a, char const *b)
   {
      return std::strcmp(a, b) < 0;
   }
};
typedef std::map<char *,int,cmp_str> aMap;

struct __perFasta_t{
  faidx_t *fai;//contains the faidx structure
  char **seqs;//contains the reference[chr][position]
  int *chrLen;//length of chromosome
  aMap asso;
};


//this will initialize our data
perFasta *init(const char *fname){
  fprintf(stderr,"[%s.%s:%d] %s\n",__FILE__,__FUNCTION__,__LINE__,fname);
  perFasta *r= new perFasta;
  r->fai = NULL;
  r->seqs = NULL;
  
  if(NULL==(r->fai = fai_load(fname))){
    fprintf(stderr,"[%s:%s] error reading fai file:%s\n",__FILE__,__FUNCTION__,fname);
    exit(0);
  }
  int nref = faidx_nseq(r->fai);
  r->chrLen = new int[nref];
  r->seqs = new char*[nref];
  for(int i=0;i<nref;i++)
    r->seqs[i] = NULL;
  return r;
}


//this will destroy a perfasta structure
void destroy(perFasta *f){
  fai_destroy(f->fai);
  int i=0;
  for(aMap::iterator it=f->asso.begin();it!=f->asso.end();++it){
    free(f->seqs[i++]);
    free(it->first);
  }
  delete [] f->seqs;
  delete [] f->chrLen;
  delete f;
  f=NULL;
}

int loadChr(perFasta *f, char*chrName){
  //  fprintf(stderr,"[%s] \t->loading chr:%s from faidx=%p cursize=%zu\n",__FUNCTION__,chrName,f,f->asso.size());
  fflush(stderr);
  f->asso.insert(std::pair<char*,int>(strdup(chrName),f->asso.size()));
  
  
  f->seqs[f->asso.size()-1] = faidx_fetch_seq(f->fai, chrName, 0, 0x7fffffff, &f->chrLen[f->asso.size()-1]);
  if(f->seqs[f->asso.size()-1]==NULL){
    fprintf(stderr,"[%s] Error loading fasta info from chr:\'%s\', will set all positions to \'N\' for this chromosome \n",__FUNCTION__,chrName);  
    f->chrLen[f->asso.size()-1] = 0;
  }
  //  fprintf(stderr,"[%s] done\n",__FUNCTION__);
  return f->asso.size()-1;
}

//this assumes that the pos is zero indexed
char getchar(char *chr,int pos,perFasta *ref){
  
  static int verbCounter =20;
  aMap::const_iterator it = ref->asso.find(chr);
  int which = -1;
  if(it==ref->asso.end()){
    which = loadChr(ref,chr);
  }else
    which = it->second;
  char refchar = 'N';
  if(pos<0) //<-strange case of nonsense position
    return refchar;
  if(pos < ref->chrLen[which])
    refchar = ref->seqs[which][pos];
  else
    if(verbCounter-->0)
      fprintf(stderr,"Problem try to access after end of chr: \'%s\' with pos: %d reflength is only:%d char set to 'N'\n",chr,pos,ref->chrLen[which]);
  return refchar;
}

