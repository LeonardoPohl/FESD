# %% [markdown]
# # FESDModel - Model Evaluation
# 
# FESD - Fault estimation for skeleton detection - is a suite that aims at finding faults in joints of skeletons, which are detected by human pose estimatiors.
# 
# FESDData is the sister project to this notebook, which aims at recording depth and rgb data, as well as populating the data with human poses from variing human pose estimators.
# 
# Furthermore, FESTData augments all data based on joint confidence.
# 
# FFESDModel aims to develop and evaluate a model based on the faulty and augmented joint data as well as RGBD data.

import os
from pathlib import Path
from time import time

from data import FESDDataset
from data import Frame, AugmentationParams
import json
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
pd.options.mode.chained_assignment = None
import seaborn as sns
sns.set()

import cv2

from model import FESD, train, val, test
import copy

import scipy
from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay

import datetime

import torch

from utils.mode import Mode
from utils import err2gt, gt2err

from tqdm.notebook import tqdm

RECORDING_DIR = Path('D:/Recordings/')
CHECKPOINT_DIR = Path('checkpoints')

# ## Metadata Loading
# 
# Firstly we need to import all the recordings into the notebook.
# 

with open(file="Exercises.json", mode='r') as file:
  exercises_json = json.load(file)['Exercises']

with open(file="JointErrors.json", mode='r') as file:
  joint_error_json = json.load(file)

with open(file="SkeletonErrors.json", mode='r') as file:
  skeleton_error_json = json.load(file)

len(exercises_json)

joint_names_all = ["-", "Head", "Neck", "Torso", "Waist", "Left collar", "Left shoulder", "Left ebpow", "Left wrist", "Left hand", "-", "Right collar", "Right shoulder", "Right elbow", "Right wrist", "Right hand", "-", "Left hip", "Left knee", "Left ankle", "-", "Right hip", "Right knee", "Right ankle", "-"]
joint_names = [i for i in joint_names_all if i != '-']

body_halves = np.array(["Upper Half", "Lower Half"])
body_parts = np.array(["Head", "Torso", "Left arm", "Right arm", "Left leg", "Right leg"])

upper_body_i = [0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
lower_body_i = [3, 14, 15, 16, 17, 18, 19]

torso_i     = [2, 3, 4, 9]
head_i      = [0, 1]
left_arm_i  = [5, 6, 7, 8]
right_arm_i = [10, 11, 12, 13]
left_leg_i  = [14, 15, 16]
right_leg_i = [17, 18, 19]

joint_errors = []
for je in joint_error_json:
  joint_errors.append(je["Name"])

# ## Model Evaluation
# 
# Here we create all graphs and analysis for the training

def prepare_result_dir(result_dir):
  result_dir.mkdir(parents=True, exist_ok=True)
  (result_dir / "fb").mkdir(parents=True, exist_ok=True)
  (result_dir / "hb").mkdir(parents=True, exist_ok=True)
  (result_dir / "bp").mkdir(parents=True, exist_ok=True)
  (result_dir / "jt").mkdir(parents=True, exist_ok=True)
  (result_dir / "roc").mkdir(parents=True, exist_ok=True)
  (result_dir / "confusion").mkdir(parents=True, exist_ok=True)

all_results = os.listdir("./results")
all_results

result_is = [2, 0, 1, 5, 3, 4]

for result_i in tqdm(result_is):
  result_dir = Path(f"./figures/results_all/{all_results[result_i]}")
  result_dir_general = Path(f"./figures/results/{all_results[result_i][:2]}")

  prepare_result_dir(result_dir)
  prepare_result_dir(result_dir_general)

  # Load the results of training and testing

  df_model = pd.read_parquet(f'./results/{all_results[result_i]}/ModelAnalysis.parquet.gzip')
  df_model["difficulty"] = df_model.apply(lambda x: int(x["exercise"][2]), axis=1)
  df_model["mode"] = df_model["mode"].apply(lambda x: Mode.from_str(mode_str=x))
  df_model["mode_str"] = df_model["mode"].apply(lambda x: x.to_str())
  df_model.head()

  tp = df_model["tp"]
  tn = df_model["tn"]
  fp = df_model["fp"]
  fn = df_model["fn"]

  df_model["p"] = df_model["tp"] + df_model["fp"]
  df_model["n"] = df_model["tn"] + df_model["fn"]
  df_model["p/(p+n)"] = df_model["p"] / (df_model["n"] + df_model["p"])

  coi = ["difficulty", "p/(p+n)", "accuracy", "precision", "recall", "f1", "mode_str"]
  coi_tf_pn = ["tp", "tn", "fp", "fn", "p", "n"]


  # # ### Training Evaluation

  # df_model_train = df_model#[df_model["train_test"] == "train"]

  # # #### Full Body Model

  # df_model_fb = df_model[df_model_train["mode"] == Mode.FULL_BODY]
  # df_model_fb["color"] = df_model_fb["train_test"].apply(lambda x: "C0" if x == "train" else "C1")
  # fig, axs = plt.subplots(nrows=2, ncols=2, sharey=True, sharex=True, figsize=(15, 5))

  # fb_train = df_model_fb[df_model_fb["train_test"] == "train"]
  # fb_test = df_model_fb[df_model_fb["train_test"] == "test"]

  # fb_train[["epoch", "loss"]].groupby("epoch").mean(numeric_only=True).plot(y="loss", figsize=(15, 5), title="Cross-Entropy Loss", ax=axs[0][0])
  # fb_test[["epoch", "loss"]].groupby("epoch").mean(numeric_only=True).plot(y="loss", figsize=(15, 5), title="Cross-Entropy Loss", ax=axs[0][0])

  # fb_train[["epoch", "accuracy"]].groupby("epoch").mean(numeric_only=True).plot(y="accuracy" , figsize=(15, 5), title="Accuracy", ax=axs[0][1])
  # fb_test[["epoch", "accuracy"]].groupby("epoch").mean(numeric_only=True).plot(y="accuracy" , figsize=(15, 5), title="Accuracy", ax=axs[0][1])

  # fb_train[["epoch", "f1"]].groupby("epoch").mean(numeric_only=True).plot(y="f1" , figsize=(15, 5), title="F1", ax=axs[1][0])
  # fb_test[["epoch", "f1"]].groupby("epoch").mean(numeric_only=True).plot(y="f1" , figsize=(15, 5), title="F1", ax=axs[1][0])

  # fb_train[["epoch", "p/(p+n)"]].groupby("epoch").mean(numeric_only=True).plot(y="p/(p+n)" , figsize=(15, 5), title="Positive guesses", ax=axs[1][1])
  # fb_test[["epoch", "p/(p+n)"]].groupby("epoch").mean(numeric_only=True).plot(y="p/(p+n)" , figsize=(15, 5), title="Positive guesses", ax=axs[1][1])

  # for ax in axs.flatten():
  #   ax.legend(["Train", "Test"])

  # fig.suptitle("Full Body Error Estimation")

  # fig.tight_layout(pad=.5)
  # fig.savefig(result_dir_general / "fb/FullBody_ErrorEstimation.png", dpi=300, bbox_inches='tight')
  # fig.savefig(result_dir / "fb/FullBody_ErrorEstimation.png", dpi=300, bbox_inches='tight')

  # # #### Half Body

  # df_model_hb = df_model[df_model["mode"] == Mode.HALF_BODY]
  # df_model_hb["joint"] = df_model_hb["joint_id"].apply(lambda x: body_halves[x])

  # fig_uh, axs_uh = plt.subplots(nrows=2, ncols=2, sharey=True, sharex=True, figsize=(15, 10))
  # fig_lh, axs_lh = plt.subplots(nrows=2, ncols=2, sharey=True, sharex=True, figsize=(15, 10))

  # crit_lower = df_model_hb["joint"] == "Lower Half"
  # crit_upper = df_model_hb["joint"] == "Upper Half"
  # crit_train = df_model_hb["train_test"] == "train"
  # crit_test = df_model_hb["train_test"] == "test"
  
  # crit = crit_lower & crit_train
  # vals_lh_tr = df_model_hb[crit]
  # crit = crit_lower & crit_test
  # vals_lh_te = df_model_hb[crit]
  # crit = crit_upper & crit_train
  # vals_uh_tr = df_model_hb[crit]
  # crit = crit_upper & crit_test
  # vals_uh_te = df_model_hb[crit]

  # def plot(df, axs):
  #   df = df[["epoch", "loss", "accuracy", "f1", "p/(p+n)", "Avg loss"]].groupby(["epoch"]).mean(numeric_only=True)
    
  #   df.plot(y="loss", figsize=(15, 5), title="Loss", ax=axs[0][0])
  #   df.plot(y="accuracy" , figsize=(15, 5), title="Accuracy", ax=axs[0][1])
  #   df.plot(y="f1" , figsize=(15, 5), title="F1", ax=axs[1][0])
  #   df.plot(y="p/(p+n)" , figsize=(15, 5), title="Positive guesses", ax=axs[1][1])

  # plot(vals_lh_tr, axs_lh)
  # plot(vals_lh_te, axs_lh)

  # for ax in axs_lh.flatten():
  #   ax.legend(["Train", "Test"])

  # fig_lh.suptitle("Lower Half Error Estimation")
  # fig_lh.tight_layout(pad=.5)
  # fig_lh.savefig(result_dir_general / "hb/LowerBody_ErrorEstimation.png", dpi=300, bbox_inches='tight')
  # fig_lh.savefig(result_dir / "hb/LowerBody_ErrorEstimation.png", dpi=300, bbox_inches='tight')

  # plot(vals_uh_tr, axs_uh)
  # plot(vals_uh_te, axs_uh)

  # for ax in axs_uh.flatten():
  #   ax.legend(["Train", "Test"])

  # fig_uh.suptitle("Upper Half Error Estimation")

  # fig_uh.tight_layout(pad=.5)
  # fig_uh.savefig(result_dir_general / "hb/UpperBody_ErrorEstimation.png", dpi=300, bbox_inches='tight')
  # fig_uh.savefig(result_dir / "hb/UpperBody_ErrorEstimation.png", dpi=300, bbox_inches='tight')
  # plt.close()

  # # #### Body parts

  # df_model_bp = df_model[df_model["mode"] == Mode.BODY_PARTS]
  # df_model_bp["joint"] = df_model_bp["joint_id"].apply(lambda x: body_parts[x])

  
  # def plot(df, axs, body_part):
  #   df = df[["epoch", "loss", "accuracy", "f1", "p/(p+n)", "Avg loss"]].groupby(["epoch"]).mean(numeric_only=True)
    
  #   df.plot(y="loss",         figsize=(15, 5), title="Cross-Entropy Loss", ax=axs[0][0], legend=False)
  #   df.plot(y="accuracy",     figsize=(15, 5), title="Accuracy", ax=axs[0][1], legend=False)
  #   df.plot(y="f1",           figsize=(15, 5), title="F1", ax=axs[1][0], legend=False)
  #   df.plot(y="p/(p+n)" ,     figsize=(15, 5), title="Positive guesses", ax=axs[1][1], legend=False)

  # for body_part in body_parts:
  #   fig, axs = plt.subplots(nrows=2, ncols=2, sharex=True, sharey=True, figsize=(15, 10))

  #   crit_1 = df_model_bp["joint"] == body_part
  #   crit_train = df_model_bp["train_test"] == "train"
  #   crit_test = df_model_bp["train_test"] == "test"
  #   crit = crit_1 & crit_train
  #   df_train = df_model_bp[crit]
  #   crit = crit_1 & crit_test
  #   df_test = df_model_bp[crit]

  #   plot(df_train, axs, body_part)
  #   plot(df_test, axs, body_part)
    
  #   for ax in axs.flatten():
  #     ax.legend(["Train", "Test"])
      
  #   #fig.tight_layout(pad=.5)
  #   fig.suptitle(f"Body part Error Estimation ({body_part})")
  #   fig.savefig(result_dir_general / f"bp/{body_part}_ErrorEstimation.png", dpi=300, bbox_inches='tight')
  #   fig.savefig(result_dir / f"bp/{body_part}_ErrorEstimation.png", dpi=300, bbox_inches='tight')
  #   plt.close()

  # # #### Joints

  # df_model_jt = df_model[df_model["mode"] == Mode.JOINTS]
  # df_model_jt["joint"] = df_model_jt["joint_id"].apply(lambda x: joint_names[x])

  # def plot(df, axs):
  #   df = df[["epoch", "loss", "accuracy", "f1", "p/(p+n)", "Avg loss"]].groupby(["epoch"]).mean(numeric_only=True)
    
  #   df.plot(y="loss", figsize=(15, 5), title="Loss", ax=axs[0][0], legend=False)
  #   df.plot(y="accuracy" , figsize=(15, 5), title="Accuracy", ax=axs[0][1], legend=False)
  #   df.plot(y="f1" , figsize=(15, 5), title="F1", ax=axs[1][0], legend=False)
  #   df.plot(y="p/(p+n)" , figsize=(15, 5), title="Positive guesses", ax=axs[1][1], legend=False)


  # for joint in joint_names:
  #   fig, axs = plt.subplots(nrows=2, ncols=2, sharex=True, sharey=True, figsize=(15, 10))

  #   crit = df_model_jt["joint"] == joint
  #   df_joint = df_model_jt[crit]
    
  #   df_train = df_joint[df_joint["train_test"] == "train"]
  #   df_test = df_joint[df_joint["train_test"] == "test"]

  #   plot(df_train, axs)
  #   plot(df_test, axs)
  #   for ax in axs.flatten():
  #     ax.legend(["Train", "Test"])

  #   fig.suptitle(f"Joint Error Estimation ({joint.replace('ebpow', 'elbow')})")
  #   fig.savefig(result_dir_general / f"jt/{joint}_ErrorEstimation.png", dpi=300, bbox_inches='tight')
  #   fig.savefig(result_dir / f"jt/{joint}_ErrorEstimation.png", dpi=300, bbox_inches='tight')
  #   plt.close()

  

  # # ### Test Evaluation

  # df_model_test = df_model[df_model["train_test"] == "test"][df_model["epoch"] == 50]

  # confusion = df_model_test[["mode_str", "tp","tn","fp","fn"]].groupby("mode_str").sum()
  # tp = confusion["tp"]
  # tn = confusion["tn"]
  # fp = confusion["fp"]
  # fn = confusion["fn"]

  # groups = df_model_test[coi].groupby("mode_str").mean(numeric_only=True)
  # groups = groups.drop(["difficulty", "precision", "recall"], axis=1)
  # groups["p/(p+n)"] *= 100
  # groups["binary_kappa"] = (2 * (tp * tn) - (fn * fp)) / ((tp + fp) * (fp + tn) + (tp + fn) * (fn + tn))

  # table = groups.rename(columns={
  #         "p/(p+n)": "Percentage of positive guesses",
  #         "accuracy": "Accuracy",
  #         "f1": "F1-Score",
  #         "binary_kappa": "Cohen's Kappa Coefficient"
  #       },
  #       index=lambda id: id.replace("_", " ").title()
  #   ).to_latex(float_format="{:.3f}".format, escape=False).replace("toprule", "hline").replace("midrule", "hline").replace("bottomrule", "hline").replace("mode_str","Problem Set")

  # table = """\\begin{table}
  #     \\caption[]{}
  #     \\label{}
  #     """ + table + "\\end{table}"

  # with open(result_dir / "res_table.tex", "w") as f:
  #   f.write(table)
  # with open(result_dir_general / "res_table.tex", "w") as f:
  #   f.write(table)


  # # #### Full Body Evaluation
  # # 
  # # The evaluation of the whole body as an error boolean.

  # df_model_full_body_test = df_model_test[df_model_test["mode"] == Mode.FULL_BODY]
  # df_model_full_body_test[coi].groupby(["difficulty"]).mean(numeric_only=True).sort_values(by="difficulty", ascending=False)
  # df_model_full_body_test[coi_tf_pn + ["difficulty"]].groupby(["difficulty"]).sum()

  # fig, axs = plt.subplots(1, 4, figsize=(7, 2.5), sharey=True)
  # fig.suptitle(f"Full Body")
  # difficulty_names = ["Trivial", "Easy", "Medium", "Hard"]
  # for difficulty in [0, 1, 2, 3]:
  #   gts = ["No Error" if x[0] == 0 else "Error" for x in df_model_full_body_test[df_model_full_body_test["difficulty"] == difficulty]["gts"].values.tolist()]
  #   preds = ["No Error" if x[0] == 0 else "Error" for x in df_model_full_body_test[df_model_full_body_test["difficulty"] == difficulty]["preds"].values.tolist()]
  #   cm = confusion_matrix(gts, preds, labels=["No Error", "Error"])
  #   disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["No Error", "Error"])
  #   disp.plot(ax=axs[difficulty], colorbar=False)
  #   axs[difficulty].set_title(difficulty_names[difficulty])
  #   axs[difficulty].grid(False)
  #   if difficulty != 0:
  #     axs[difficulty].set_ylabel("")

  # fig.savefig(result_dir_general / "confusion/full_difficulty.png", dpi=300, bbox_inches='tight')
  # fig.savefig(result_dir / "confusion/full_difficulty.png", dpi=300, bbox_inches='tight')
  # plt.close()
  # def single_confusion(df, result_dir, ps_name, file_name):
  #   fig, axs = plt.subplots(1, 1, figsize=(7, 2.5), sharey=True)

  #   gts = ["No Error" if x[0] == 0 else "Error" for x in df[df["difficulty"] == difficulty]["gts"].values.tolist()]
  #   preds = ["No Error" if x[0] == 0 else "Error" for x in df[df["difficulty"] == difficulty]["preds"].values.tolist()]
  #   cm = confusion_matrix(gts, preds, labels=["No Error", "Error"])
  #   disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["No Error", "Error"])
  #   disp.plot(ax=axs, colorbar=False)

  #   axs.grid(False)

  #   fig.suptitle(f"{ps_name}\nConfusion Matrix")
  #   plt.tight_layout()

  #   fig.savefig(result_dir_general / f"confusion/{file_name}", dpi=300, bbox_inches='tight')
  #   fig.savefig(result_dir / f"confusion/{file_name}", dpi=300, bbox_inches='tight')
  #   plt.close()

  # single_confusion(df_model_full_body_test, result_dir,"Full Body", "full_together.png")
  

  # # #### Half Body Evaluation
  # # 
  # # The evaluation of the body split into two parts (upper and lower body), each as an error boolean.

  # # Split the data

  # df_model_half_body_test = df_model_test[df_model_test["mode"] == Mode.HALF_BODY]
  # df_model_half_body_test["joint_names"] = df_model_half_body_test["joint_id"].apply(lambda x: body_halves[x])
  # df_model_half_body_test[coi + ["joint_names"]].groupby(["joint_names", "difficulty"]).mean(numeric_only=True)
  # df_model_half_body_test[coi_tf_pn + ["joint_names", "difficulty"]].groupby(["joint_names", "difficulty"]).sum()
  # single_confusion(df_model_half_body_test, result_dir, "Half Body", "half_together.png")

  # df = df_model_half_body_test
  # areas = body_halves

  # rows = 1
  # cols = 2
  # assert len(areas) == rows * cols

  # figure = plt.figure(layout='constrained', figsize=(4 * cols, 4.5 * rows))
  # subfigs = figure.subfigures(rows, cols)

  # for i, area in enumerate(areas):
  #   axs = subfigs.flatten()[i].subplots(1, 1)
  #   #subfigs.flatten()[i].suptitle(area, fontsize='x-large')

  #   crit = df["joint_names"] == area
  #   gts = ["No Error" if x[0] == 0 else "Error" for x in df[crit]["gts"].values.tolist()]
  #   preds = ["No Error" if x[0] == 0 else "Error" for x in df[crit]["preds"].values.tolist()]

  #   cm = confusion_matrix(gts, preds, labels=["No Error", "Error"])
  #   disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["No Error", "Error"])
  #   disp.plot(ax=axs, colorbar=False)

  #   axs.grid(False)
  #   axs.set_title(area, fontsize='x-large')

  # #plt.tight_layout()
  # figure.savefig(result_dir_general / "confusion/body_halves_half.png", dpi=300, bbox_inches='tight')
  # figure.savefig(result_dir / "confusion/body_halves_half.png", dpi=300, bbox_inches='tight')
  # plt.close()
  # figure = plt.figure(layout='constrained', figsize=(7, 2.5 * len(body_halves)))
  # subfigs = figure.subfigures(2, 1)

  # difficulty_names = ["Trivial", "Easy", "Medium", "Hard"]
  # for i, half in enumerate(body_halves):
  #   axs = subfigs[i].subplots(1, 4, sharey=True)
  #   subfigs[i].suptitle(half, fontsize='x-large')
  #   for difficulty in [0, 1, 2, 3]:  
  #     crit_1 = df_model_half_body_test["difficulty"] == difficulty
  #     crit_2 = df_model_half_body_test["joint_names"] == half
  #     crit = crit_1 & crit_2
  #     gts = ["No Error" if x[0] == 0 else "Error" for x in df_model_half_body_test[crit]["gts"].values.tolist()]
  #     preds = ["No Error" if x[0] == 0 else "Error" for x in df_model_half_body_test[crit]["preds"].values.tolist()]

  #     cm = confusion_matrix(gts, preds, labels=["No Error", "Error"])
  #     disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["No Error", "Error"])
  #     disp.plot(ax=axs[difficulty], colorbar=False)

  #     axs[difficulty].grid(False)
  #     axs[difficulty].set_title(difficulty_names[difficulty])
      
  #     if i < len(body_halves) - 1:
  #       axs[difficulty].set_xlabel("") 
  #       axs[difficulty].set_xticks([]) 
  #     if difficulty != 0:
  #       axs[difficulty].set_ylabel("")

  # #plt.tight_layout()
  # figure.savefig(result_dir_general / "confusion/body_halves_difficulty.png", dpi=300, bbox_inches='tight')
  # figure.savefig(result_dir / "confusion/body_halves_difficulty.png", dpi=300, bbox_inches='tight')
  # plt.close()

  # # #### Body part Evaluation
  # # 
  # # The evaluation of the body split into body_parts (Head, Torso, Left and Right Arm, and Left and Right Leg), each as an error boolean.

  #   # Split the data

  # df_model_body_parts_test = df_model_test[df_model_test["mode"] == Mode.BODY_PARTS]
  # df_model_body_parts_test["joint_names"] = df_model_body_parts_test["joint_id"].apply(lambda x: body_parts[x])
  # df_model_body_parts_test[coi + ["joint_names"]].groupby(["joint_names", "difficulty"]).mean(numeric_only=True)
  # df_model_body_parts_test[coi_tf_pn + ["joint_names", "difficulty"]].groupby(["joint_names", "difficulty"]).sum()

  # single_confusion(df_model_body_parts_test, result_dir, "Body Parts", "body_parts_together.png")

  # rows = 2
  # cols = 3

  # figure = plt.figure(layout='constrained', figsize=(4 * cols, 4.5 * rows))
  # subfigs = figure.subfigures(rows, cols)

  # for i, body_part in enumerate(body_parts):
  #   axs = subfigs.flatten()[i].subplots(1, 1)

  #   crit = df_model_body_parts_test["joint_names"] == body_part
  #   gts = ["No Error" if x[0] == 0 else "Error" for x in df_model_body_parts_test[crit]["gts"].values.tolist()]
  #   preds = ["No Error" if x[0] == 0 else "Error" for x in df_model_body_parts_test[crit]["preds"].values.tolist()]

  #   cm = confusion_matrix(gts, preds, labels=["No Error", "Error"])
  #   disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["No Error", "Error"])
  #   disp.plot(ax=axs, colorbar=False)

  #   axs.grid(False)
  #   axs.set_title(body_part, fontsize='x-large')

  # #plt.tight_layout()
  # figure.savefig(result_dir_general / "confusion/body_parts_part.png", dpi=300, bbox_inches='tight')
  # figure.savefig(result_dir / "confusion/body_parts_part.png", dpi=300, bbox_inches='tight')
  # plt.close()
  # rows = 3
  # cols = 2
  # assert rows * cols == len(body_parts)

  # figure = plt.figure(layout='constrained', figsize=(7 * cols, 2.5 * rows))
  # subfigs = figure.subfigures(nrows=rows, ncols=cols)

  # difficulty_names = ["Trivial", "Easy", "Medium", "Hard"]
  # for i, body_part in enumerate(body_parts):
  #   axs = subfigs.flatten()[i].subplots(1, 4, sharey=True)
  #   subfigs.flatten()[i].suptitle(body_part, fontsize='x-large')
  #   for difficulty in [0, 1, 2, 3]:  
  #     crit_1 = df_model_body_parts_test["difficulty"] == difficulty
  #     crit_2 = df_model_body_parts_test["joint_names"] == body_part
  #     crit = crit_1 & crit_2
  #     gts = ["No Error" if x[0] == 0 else "Error" for x in df_model_body_parts_test[crit]["gts"].values.tolist()]
  #     preds = ["No Error" if x[0] == 0 else "Error" for x in df_model_body_parts_test[crit]["preds"].values.tolist()]

  #     cm = confusion_matrix(gts, preds, labels=["No Error", "Error"])
  #     disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["No Error", "Error"])
  #     disp.plot(ax=axs[difficulty], colorbar=False)

  #     axs[difficulty].grid(False)
  #     axs[difficulty].set_title(difficulty_names[difficulty])
  #     if i < len(body_parts) - 2:
  #       axs[difficulty].set_xlabel("") 
  #       axs[difficulty].set_xticks([]) 
  #     if difficulty != 0 or i % 2 == 1:
  #       axs[difficulty].set_ylabel("")
  #       axs[difficulty].set_yticks([]) 

  # #plt.tight_layout()
  # figure.savefig(result_dir_general / "confusion/body_parts_difficulty.png", dpi=300, bbox_inches='tight')
  # figure.savefig(result_dir / "confusion/body_parts_difficulty.png", dpi=300, bbox_inches='tight')
  # plt.close()

  # # #### Joint Evaluation
  # # 
  # # The evaluation of the body as an error class.

  #   # Split the data

  # df_model_joints_test = df_model_test[df_model_test["mode"] == Mode.JOINTS]
  # df_model_joints_test["joint_names"] = df_model_joints_test["joint_id"].apply(lambda x: joint_names[x])
  # df_model_joints_test[coi + ["joint_names"]].groupby(["joint_names", "difficulty"]).mean(numeric_only=True)
  # df_model_joints_test[coi_tf_pn + ["difficulty", "joint_names"]].groupby(["joint_names", "difficulty"]).sum()
  # df_model_joints_test[coi + ["joint_names"]].groupby(["joint_names", "difficulty"]).mean(numeric_only=True)
  # single_confusion(df_model_joints_test, result_dir, "Joints", "joints_together.png")

  # rows = 5
  # cols = 4
  # assert rows * cols == len(joint_names)

  # figure = plt.figure(layout='constrained', figsize=(4 * cols, 4.5 * rows))
  # subfigs = figure.subfigures(rows, cols)

  # for i, body_part in enumerate(joint_names):
  #   axs = subfigs.flatten()[i].subplots(1, 1)

  #   crit = df_model_joints_test["joint_names"] == body_part
  #   gts = ["No Error" if x[0] == 0 else "Error" for x in df_model_joints_test[crit]["gts"].values.tolist()]
  #   preds = ["No Error" if x[0] == 0 else "Error" for x in df_model_joints_test[crit]["preds"].values.tolist()]

  #   cm = confusion_matrix(gts, preds, labels=["No Error", "Error"])
  #   disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["No Error", "Error"])
  #   disp.plot(ax=axs, colorbar=False)

  #   axs.grid(False)
  #   axs.set_title(body_part, fontsize='x-large')

  # #plt.tight_layout()
  # figure.savefig(result_dir_general / "confusion/joints_joint.png", dpi=300, bbox_inches='tight')
  # figure.savefig(result_dir / "confusion/joints_joint.png", dpi=300, bbox_inches='tight')
  # plt.close()
  # rows = 5
  # cols = 4
  # assert rows * cols == len(joint_names)

  # figure = plt.figure(layout='constrained', figsize=(7 * cols, 2.5 * rows))
  # subfigs = figure.subfigures(nrows=rows, ncols=cols)

  # difficulty_names = ["Trivial", "Easy", "Medium", "Hard"]
  # for i, joint_name in enumerate(joint_names):
  #   axs = subfigs.flatten()[i].subplots(1, 4, sharey=True)
  #   subfigs.flatten()[i].suptitle(joint_name, fontsize='x-large')
  #   for difficulty in [0, 1, 2, 3]:  
  #     crit_1 = df_model_joints_test["difficulty"] == difficulty
  #     crit_2 = df_model_joints_test["joint_names"] == joint_name
  #     crit = crit_1 & crit_2
  #     gts = ["No Error" if x[0] == 0 else "Error" for x in df_model_joints_test[crit]["gts"].values.tolist()]
  #     preds = ["No Error" if x[0] == 0 else "Error" for x in df_model_joints_test[crit]["preds"].values.tolist()]

  #     cm = confusion_matrix(gts, preds, labels=["No Error", "Error"])
  #     disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["No Error", "Error"])
  #     disp.plot(ax=axs[difficulty], colorbar=False)

  #     axs[difficulty].grid(False)
  #     axs[difficulty].set_title(difficulty_names[difficulty])
  #     if i < (rows - 1) * cols:
  #       axs[difficulty].set_xlabel("") 
  #       axs[difficulty].set_xticks([]) 
  #     if difficulty != 0 or i % cols != 0:
  #       axs[difficulty].set_ylabel("")
  #       axs[difficulty].set_yticks([]) 

  # #plt.tight_layout()
  # figure.savefig(result_dir_general / "confusion/joints_difficulty.png", dpi=300, bbox_inches='tight')
  # figure.savefig(result_dir / "confusion/joints_difficulty.png", dpi=300, bbox_inches='tight')
  # plt.close()
  # plt.close()
  # plt.close()
  # plt.close()
  # plt.close()
  # plt.close()

  # ## ROC

  from sklearn.metrics import roc_curve, roc_auc_score

  def plot_roc_auc(y_true, y_score, obj, ax, i):
    if i == 0:
      ns_probs = [0 for _ in range(len(y_true))]
      ns_auc = roc_auc_score(y_true, ns_probs)
      ns_fpr, ns_tpr, _ = roc_curve(y_true, ns_probs, drop_intermediate=False)
      ax.plot(ns_fpr, ns_tpr, linestyle='--', label='No Skill', color="C0")
    lr_auc = roc_auc_score(y_true, y_score)
    # summarize scores
    #print('ROC AUC=%.3f' % (lr_auc))
    # calculate roc curves
    lr_fpr, lr_tpr, thresh = roc_curve(y_true, y_score, drop_intermediate=False)
    
    # plot the roc curve for the model
    ax.plot(lr_fpr, lr_tpr, marker='.', label=obj, color=f"C{i+1}")
    # axis labels
    ax.set_xlabel('False Positive Rate')
    ax.set_ylabel('True Positive Rate')
    # show the legend
    ax.legend()

  # ### Full Body
  fig, ax = plt.subplots()
  df_fb = df_model[df_model["mode"] == Mode.FULL_BODY]
  df_fb = df_fb[df_fb["train_test"] == "test"]
  df_fb = df_fb[df_fb["epoch"] == 50]
  y_true = (df_fb["gts"].apply(lambda x: int(x[0]))).to_list()
  y_score = (df_fb["confidences"].apply(lambda x: x[0])).to_list()

  plot_roc_auc(y_true, y_score, "Full Body", ax, 0)
  fig = plt.gcf()
  fig.savefig(result_dir_general / "roc/fb.png", dpi=300, bbox_inches='tight')
  fig.savefig(result_dir / "roc/fb.png", dpi=300, bbox_inches='tight')
  plt.close()
  
  # ## Half Body

  fig, ax = plt.subplots()
  for i, obj in enumerate(body_halves):
    df_hb = df_model[df_model["mode"] == Mode.HALF_BODY]
    df_hb = df_hb[df_hb["joint_id"] == i]
    df_hb = df_hb[df_hb["train_test"] == "test"]
    df_hb = df_hb[df_hb["epoch"] == 50]
    y_true = (df_hb["gts"].apply(lambda x: int(x[0]))).to_numpy()
    y_score = (df_hb["confidences"].apply(lambda x: x[0])).to_numpy()

    plot_roc_auc(y_true, y_score, obj, ax, i)

  fig = plt.gcf()
  fig.savefig(result_dir_general / "roc/hb.png", dpi=300, bbox_inches='tight')
  fig.savefig(result_dir / "roc/hb.png", dpi=300, bbox_inches='tight')
  plt.close()

  # ## Body Parts

  fig, ax = plt.subplots()
  for i, obj in enumerate(body_parts):
    df = df_model[df_model["mode"] == Mode.BODY_PARTS]
    df = df[df["joint_id"] == i]
    df = df[df["train_test"] == "test"]
    df = df[df["epoch"] == 50]
    y_true = (df["gts"].apply(lambda x: int(x[0]))).to_numpy()
    y_score = (df["confidences"].apply(lambda x: x[0])).to_numpy()

    plot_roc_auc(y_true, y_score, obj, ax, i)
    
  fig.savefig(result_dir_general / "roc/bp.png", dpi=300, bbox_inches='tight')
  fig.savefig(result_dir / "roc/bp.png", dpi=300, bbox_inches='tight')
  plt.close()
  
  # ## Joints

  fig, ax = plt.subplots()
  for i, obj in enumerate(joint_names):
    df = df_model[df_model["mode"] == Mode.JOINTS]
    df = df[df["joint_id"] == i]
    df = df[df["train_test"] == "test"]
    df = df[df["epoch"] == 50]
    y_true = (df["gts"].apply(lambda x: int(x[0] != 0))).to_numpy()
    y_score = (df["confidences"].apply(lambda x: x[0])).to_numpy()

    plot_roc_auc(y_true, y_score, obj, ax, i)
    
  plt.legend(loc='upper center', bbox_to_anchor=(0.5, -0.15), ncol=3)
  fig = plt.gcf()
  fig.savefig(result_dir_general / "roc/jt.png", dpi=300, bbox_inches='tight')
  fig.savefig(result_dir / "roc/jt.png", dpi=300, bbox_inches='tight')
  plt.close()
