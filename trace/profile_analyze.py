import pandas as pd

df = pd.read_csv('data.csv')

df.groupby('library').sum('count').to_csv('data_calls_by_lib.csv')

syscalls = pd.read_csv('trace/syscalls.csv')
df[df['function'].isin(syscalls.syscall.to_list())].to_csv('data_syscalls.csv', index=False)