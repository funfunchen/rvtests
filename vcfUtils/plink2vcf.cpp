#include "Argument.h"
#include "IO.h"
#include "PlinkInputFile.h"
#include "SimpleMatrix.h"

int laodReference(const std::string& FLAG_reference,
                  std::map<std::string, char>* reference) {
  if (FLAG_reference.empty()) return 0;

  reference->clear();

  LineReader lr(FLAG_reference);
  std::vector<std::string> fd;
  while (lr.readLineBySep(&fd, "\t ")) {
    if (reference->count(fd[0])) {
      fprintf(stderr, "Duplicated rsNumber %s\n", fd[0].c_str());
    };
    if (fd.size() < 2 || fd[1].size() != 1) {
      fprintf(stderr,
              "Incorrect columns number or alleles length not equal to 1\n");
      continue;
    };
    (*reference)[fd[0]] = fd[1][0];
  }
  return reference->size();
};

////////////////////////////////////////////////
BEGIN_PARAMETER_LIST()
ADD_PARAMETER_GROUP("Input/Output")
ADD_STRING_PARAMETER(inPlink, "--inPlink", "input Plink File")
ADD_STRING_PARAMETER(outVcf, "--outVcf", "output prefix")
ADD_STRING_PARAMETER(reference, "--reference",
                     "reference file (format: "
                     "rsNumber reference_base), can "
                     "be generated by getMapRef")
// ADD_STRING_PARAMETER(outPlink, "--make-bed", "output prefix")
// ADD_PARAMETER_GROUP("People Filter")
// ADD_STRING_PARAMETER(peopleIncludeID, "--peopleIncludeID", "give IDs of
// people that will be included in study")
// ADD_STRING_PARAMETER(peopleIncludeFile, "--peopleIncludeFile", "from given
// file, set IDs of people that will be included in study")
// ADD_STRING_PARAMETER(peopleExcludeID, "--peopleExcludeID", "give IDs of
// people that will be included in study")
// ADD_STRING_PARAMETER(peopleExcludeFile, "--peopleExcludeFile", "from given
// file, set IDs of people that will be included in study")
// ADD_PARAMETER_GROUP("Site Filter")
// ADD_STRING_PARAMETER(rangeList, "--rangeList", "Specify some ranges to use,
// please use chr:begin-end format.")
// ADD_STRING_PARAMETER(rangeFile, "--rangeFile", "Specify the file containing
// ranges, please use chr:begin-end format.")
END_PARAMETER_LIST();

int main(int argc, char* argv[]) {
  time_t currentTime = time(0);
  fprintf(stderr, "Analysis started at: %s", ctime(&currentTime));

  PARSE_PARAMETER(argc, argv);
  PARAMETER_STATUS();

  if (FLAG_REMAIN_ARG.size() > 0) {
    fprintf(stderr, "Unparsed arguments: ");
    for (unsigned int i = 0; i < FLAG_REMAIN_ARG.size(); i++) {
      fprintf(stderr, " %s", FLAG_REMAIN_ARG[i].c_str());
    }
    fprintf(stderr, "\n");
    abort();
  }

  REQUIRE_STRING_PARAMETER(FLAG_inPlink,
                           "Please provide input file using: --inPlink");

  PlinkInputFile* pin = new PlinkInputFile(FLAG_inPlink.c_str());
  FILE* fout = fopen(FLAG_outVcf.c_str(), "wt");
  FILE* flog = fopen((FLAG_outVcf + ".log").c_str(), "wt");

  int numPeople = pin->getNumIndv();
  int numMarker = pin->getNumMarker();
  fprintf(stderr, "Loaded %d individuals and %d markers\n", numPeople,
          numMarker);
  fprintf(flog, "Loaded %d individuals and %d markers\n", numPeople, numMarker);

  fprintf(fout, "##fileformat=VCFv4.0\n");
  fprintf(fout, "##filedate=\n");
  fprintf(fout, "##source=plink2vcf\n");
  fprintf(fout, "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT");

  // writer header
  for (int p = 0; p < numPeople; p++) {
    fprintf(fout, "\t%s", pin->indv[p].c_str());
  };
  fprintf(fout, "\n");

  // load reference allele
  std::map<std::string, char> reference;
  int ret = laodReference(FLAG_reference, &reference);
  if (ret) {
    fprintf(stderr, "Loaded %d referenced bases.\n", ret);
    fprintf(flog, "Loaded %d referenced bases.\n", ret);
  };
  // write content
  SimpleMatrix mat;
  ret = pin->readIntoMatrix(&mat);

  //   const char ref = 'N';
  //   const char alt = 'N';
  bool switchRefAlt;
  int switchSite = 0;
  int needFlip = 0;
  for (int m = 0; m < numMarker; m++) {
    switchRefAlt = false;
    if (reference.size() > 0 && reference.count(pin->snp[m]) > 0) {
      char refGiven = reference[pin->snp[m]];

      if (pin->ref[m][0] == refGiven) {
        switchRefAlt = false;
      } else {
        if (pin->alt[m][0] == refGiven) {
          switchRefAlt = true;
          ++switchSite;
          fprintf(flog, "Marker [ %s ] switched ref and alt.\n",
                  pin->snp[m].c_str());
        } else {
          ++needFlip;
          fprintf(flog, "Marker [ %s ] need flipping.\n", pin->snp[m].c_str());
        };
      }
    }
    if (switchRefAlt == false) {
      fprintf(fout, "%s\t", pin->chrom[m].c_str());  // CHROM
      fprintf(fout, "%d\t", pin->pos[m]);            // POS
      fprintf(fout, "%s\t", pin->snp[m].c_str());    // ID
      fprintf(fout, "%c\t", pin->ref[m][0]);         // REF
      fprintf(fout, "%c\t", pin->alt[m][0]);         // ALT
      fprintf(fout, ".\t");                          // QUAL
      fprintf(fout, ".\t");                          // FILTER
      fprintf(fout, ".\t");                          // INFO
      fprintf(fout, "GT\t");                         // FORMAT
      for (int p = 0; p < numPeople; p++) {
        if (p) fputc('\t', fout);

        int geno = mat[p][m];
        switch (geno) {
          case 0:
            fputs("0/0", fout);
            break;
          case 1:
            fputs("0/1", fout);
            break;
          case 2:
            fputs("1/1", fout);
            break;
          default:
            fputs("./.", fout);
            break;
        }
      }
      fprintf(fout, "\n");
    } else {
      fprintf(fout, "%s\t", pin->chrom[m].c_str());  // CHROM
      fprintf(fout, "%d\t", pin->pos[m]);            // POS
      fprintf(fout, "%s\t", pin->snp[m].c_str());    // ID
      fprintf(fout, "%c\t", pin->alt[m][0]);         // REF
      fprintf(fout, "%c\t", pin->ref[m][0]);         // ALT
      fprintf(fout, ".\t");                          // QUAL
      fprintf(fout, ".\t");                          // FILTER
      fprintf(fout, ".\t");                          // INFO
      fprintf(fout, "GT\t");                         // FORMAT
      for (int p = 0; p < numPeople; p++) {
        if (p) fputc('\t', fout);

        int geno = mat[p][m];
        switch (geno) {
          case 0:
            fputs("1/1", fout);
            break;
          case 1:
            fputs("0/1", fout);
            break;
          case 2:
            fputs("0/0", fout);
            break;
          default:
            fputs("./.", fout);
            break;
        }
      }
      fprintf(fout, "\n");
    }
  }

  fclose(fout);
  fclose(flog);

  if (switchSite) {
    fprintf(stderr, "%d SNPs switched ref and alt, see log file.\n",
            switchSite);
  };
  if (needFlip) {
    fprintf(stderr, "%d SNPs need flipping, see log file.\n", needFlip);
  };
  return 0;
}
