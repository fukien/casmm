import os
import pandas as pd

DATA_DIR = "../../dataset/groupby-raw/"


input_file_dict = {
	"ml-20m": os.path.join(DATA_DIR, "ml20m/ml-20m/ratings.csv")
}

output_file_dict = {
	"ml-20m": os.path.join(DATA_DIR, "ml20m-ratings.tsv")
}


for dataset in input_file_dict.keys():
	INPUT_FILE = input_file_dict[dataset]
	OUTPUT_FILE = output_file_dict[dataset]
	df_input = pd.read_csv(INPUT_FILE)

	df_output = df_input.drop(columns=df_input.columns[3:], axis=1)
	df_output.to_csv(OUTPUT_FILE, header=False, index=False, sep="\t", encoding="utf-8")