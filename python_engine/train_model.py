import pandas as pd
import joblib
from sklearn.ensemble import IsolationForest
import xgboost as xgb
import os

TRAIN_DATA_PATH = '../dataset/Train_data.csv'
MODEL_PATH = 'models/hybrid_model.pkl'

def train_model():
    print(f"Loading real dataset from {TRAIN_DATA_PATH}...")
    df = pd.read_csv(TRAIN_DATA_PATH)
    
    print(f"Dataset shape: {df.shape}")
    
    if 'class' in df.columns:
        y = (df['class'] != 'normal').astype(int)
    else:
        print("Warning: 'class' column not found. XGBoost needs labels! Defaulting to 0.")
        y = pd.Series([0]*len(df))

    categorical_cols = ['protocol_type', 'service', 'flag', 'class']
    
    cols_to_drop = [col for col in categorical_cols if col in df.columns]
    X_train = df.drop(columns=cols_to_drop)
    
    X_train.fillna(0, inplace=True)
    
    features = list(X_train.columns)
    
    print(f"Training XGBoost Classifier (The Bouncer)...")
    xgb_model = xgb.XGBClassifier(n_estimators=100, max_depth=5, learning_rate=0.1, random_state=42)
    xgb_model.fit(X_train, y)
    
    print(f"Training Isolation Forest (The Detective)...")
    if_model = IsolationForest(n_estimators=100, max_samples='auto', contamination=0.1, random_state=42)
    if_model.fit(X_train)
    
    os.makedirs('models', exist_ok=True)
    
    joblib.dump({
        'xgb_model': xgb_model, 
        'if_model': if_model, 
        'features': features
    }, MODEL_PATH)
    
    print(f"Models successfully trained and saved to {MODEL_PATH}")

if __name__ == "__main__":
    train_model()
