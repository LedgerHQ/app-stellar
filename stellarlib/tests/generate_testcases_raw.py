from dataset import SignTxTestCases, SignSorobanAuthorizationTestCases

CURRENT_DIR = __file__.rsplit("/", 1)[0]
OUTPUT_DIR = f"{CURRENT_DIR}/testcases"

print("Generating raw testcases...")
print("SignTxTestCases:")

for name, function in vars(SignTxTestCases).items():
    if isinstance(function, staticmethod):
        print(f'"{name}",')
        data = function.__func__().signature_base()
        with open(f"{OUTPUT_DIR}/{name}.raw", "wb") as f:
            f.write(data)

print("=" * 50)
print("SignSorobanAuthorizationTestCases:")

for name, function in vars(SignSorobanAuthorizationTestCases).items():
    if isinstance(function, staticmethod):
        print(f'"{name}",')
        data = function.__func__().to_xdr_bytes()
        with open(f"{OUTPUT_DIR}/{name}.raw", "wb") as f:
            f.write(data)
