import json
import sys
from cohort import Cohort


class ComplexEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, complex):
            return [obj.real, obj.imag]
        # Let the base class default method raise the TypeError
        return json.JSONEncoder.default(self, obj)

#intergrating into build crea
#create script of getting proper file
class FormattingJSON():
    # opening file reading from
    output_file = Cohort.from_io(sys.stdin.buffer)
    # get total number of cohorts for the given file
    totalCohorts = output_file.total_cohorts
    for i in range(len(totalCohorts)):
        # number of worker in the given cohort
        num_workers = output_file.total_cohorts[i].nworkers
        # the list for each worker in the given cohort
        output_list = []
        for i_workers in range(num_workers):
            each_worker = {
                "status": {
                    "value": output_file.total_cohorts[i].type[i_workers].status.value,
                    "name": output_file.total_cohorts[i].type[i_workers].status.name.upper()
                },
                "ndecoded": output_file.total_cohorts[i].type[i_workers].ndecoded,
                "workerno": output_file.total_cohorts[i].type[i_workers].workerno,
                "worker_so": output_file.total_cohorts[i].type[i_workers].worker_so,
                "len": output_file.total_cohorts[i].type[i_workers].len,
                "result": output_file.total_cohorts[i].type[i_workers].result
            }
            output_list.append(each_worker)

        # create a dictionary for each cohort
        cohorts = {
            "nworkers": num_workers,
            "inputs": output_file.total_cohorts[i].input,
            "outputs": output_list
        }
        # convert each Json to file
        to_json = json.dumps(cohorts, cls=ComplexEncoder)
        print (to_json, file=sys.stdout)

