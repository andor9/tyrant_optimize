# Requirements

1. Python 3.X with pip/Anaconda on Windows
2. Java 1.8 or higher

# Download

Download the source zip from github and unzip it. Or better clone the repo
```
$ git clone https://github.com/APN-Pucky/tyrant_optimize.git tuo
$ cd tuo/ml
```

# Install

```
tuo/ml $ pip install .
```

If you do not have administrator rights append --user to above command.

Alternatively, if some errors occur in later steps:

```
tuo/ml $ poetry install 
```



# Run it

```
python train.py --help
python train.py
```

The output should look similar to:

```
Loading database from database.yml...done
Parsing database...
100%|█████████████████████████████████████| 1705/1705 [00:20<00:00, 84.00it/s]
100%|███████████████████████████████████████████| 1/1 [00:20<00:00, 20.30s/it]
Training...
WIN......Average Error: +-1.752% (+-0.027%)
STALL....Average Error: +-0.750% (+-0.017%)
LOSS.....Average Error: +-1.225% (+-0.022%)
POINTS...Average Error: +-1.760 (+-0.031)
```

The uncertainty of the training is indicated.
You can copy the `*.pmml` files into your `data/` folder to use them in tuo now.
